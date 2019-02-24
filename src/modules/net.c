// Module for interfacing with network functions (elua_net.h)

//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "elua_net.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "lrotable.h"
#include <stdint.h>

#include "console.h" // TODO: Remove...

#include "platform_conf.h"
#ifdef BUILD_TCPIP

#ifdef BUILD_PICOTCP 
#include "elua_picotcp.h"
#endif

#define NET_META_NAME      "eLua.net"
#define NET_SOCK_T_NAME    "eLua.net.socketTable"
#define NET_CALLBACK_T_NAME "eLua.net.callbackTable"
#define NET_WEAK_META_NAME  "eLua.net.weakmetaTable"
#define THREAD_KEY "eLua.net.thread"


// Macros for checking if a socket returned from the stack is a valid socket
#ifdef BUILD_PICOTCP
#define stack_s_valid(s) ((intptr_t)s!=0)
#else // UIP....
#define stack_s_valid(s) ((int)s!= -1)
#endif 


typedef struct
{
  uintptr_t sock; // Attention: How to deal with 64Bit Pointers?
  lua_State *L; 
} sock_t;


#define sock_check( L, idx ) ( sock_t* )luaL_checkudata( L, idx, NET_META_NAME )
#define lua_puship( L, ip )   lua_pushnumber( L, ( lua_Number )ip )

static lua_State *G_state =NULL; 


static void create_table(lua_State *L, const char* registryKey, bool fWeak)
{
   lua_pushstring(L, registryKey  ); 
   lua_newtable(L); 
   // Add metatable
   if (fWeak) {
     lua_getfield(L,LUA_REGISTRYINDEX,NET_WEAK_META_NAME);
     if (lua_isnil(L,-1)) { // No Metatable yet, construct it
       lua_pop(L,1); // clean stack
       lua_pushstring(L, NET_WEAK_META_NAME ); // Registy key 
       lua_newtable(L);
       lua_pushstring( L,"__mode" );
       lua_pushstring( L,"v" );
       lua_settable(L,-3); 
       lua_settable(L, LUA_REGISTRYINDEX);  // Store Metatable in registry for sharing
       lua_getfield(L,LUA_REGISTRYINDEX,NET_WEAK_META_NAME); // Read it again
     }
     lua_setmetatable( L, -2 ); 
   }
   
   lua_settable(L, LUA_REGISTRYINDEX); 
}


static void  init_socket_table(lua_State *L) 
{
 
   // Create Thread for callbacks 
   lua_pushstring(L, THREAD_KEY);
   G_state = lua_newthread(L);
   lua_settable(L,LUA_REGISTRYINDEX); 
   lua_pop(L,1); // Pop thread from stack


   // Create an empty table and store in the registry 
   create_table(L,NET_SOCK_T_NAME,true);
   create_table(L,NET_CALLBACK_T_NAME,false); 
}


static void register_socket_callback(lua_State *L,uintptr_t socket,int idx)
{
   if (!lua_isfunction(L,idx)) {
     luaL_error(L,"function expected at index %d",idx);
   }
   lua_getfield(L,LUA_REGISTRYINDEX,NET_CALLBACK_T_NAME);
   lua_pushlightuserdata(L,(void*)socket);
   lua_pushvalue(L,idx); 
   lua_settable(L,-3);
}




static sock_t *push_socket( lua_State *L, uintptr_t sock )
{
    if (stack_s_valid(sock)) {
      // Create userdata object for socket
      sock_t  *s = ( sock_t* )lua_newuserdata( L, sizeof( sock_t ) );
      s->sock = sock;
      s->L=L;
      luaL_getmetatable( L, NET_META_NAME );
      lua_setmetatable( L, -2 );  
      // userdata is now on TOS

      // Construct: 
      // registry[NET_SOCK_T_NAME] = {
      //    <lightuserdata socket adr> =  <elua.net obj> 
      // }

      // Add to socket_table
      lua_getfield(L,LUA_REGISTRYINDEX,NET_SOCK_T_NAME); // registry[NET_SOCK_T_NAME]  
      lua_pushlightuserdata(L,(void*)sock); // the socket handle is the key 
      lua_pushvalue(L, -3 ); // Dup our user data
      lua_settable(L,-3);
      lua_pop(L,1); // remove table from TOS, we dont need it 
      if (!lua_isuserdata(L,-1)) kassert_fail("push_socket no userdata at tos\n");

      return s;
    } else {
      lua_pushnil( L );
      return NULL;
    }
}


static int net_get_sockettable(lua_State *L)
{
   lua_getfield(L,LUA_REGISTRYINDEX,NET_SOCK_T_NAME); // registry[NET_SOCK_T_NAME]
   lua_getfield(L,LUA_REGISTRYINDEX,NET_CALLBACK_T_NAME); //  // registry[NET_CALLBACK_T_NAME]
   return 2; 
}

static sock_t  *get_socket( lua_State *L, int idx )
{
  sock_t *s = sock_check( L, idx );
  if (s && s->sock== 0 ) {
     luaL_error( L, "elua.net: Socket %p already closed",s );
     return NULL; 
  }
  return s;     

}


// maps socket pointer to the userdata socket object
// Attention: leaves userdata on stack to avoid being collected
static sock_t *map_socket(lua_State *L,uintptr_t s)
{
   lua_getfield(L,LUA_REGISTRYINDEX,NET_SOCK_T_NAME); // registry[NET_SOCK_T_NAME]
   lua_pushlightuserdata( L,(void*)s );
   lua_gettable(L,-2);
   lua_remove(L,-2); // remove table from stack
   if ( lua_isuserdata(L,-1) )
     return sock_check(L,-1);
   else
     return NULL;
}


/*
 * TH: Added listen/unlisten functions
 */

//Lua: err=listen(port)
// PICOTCP: socket=listen(port)
static int net_listen( lua_State *L )
{
  u16 port = ( u16 )luaL_checkinteger( L, 1 );
  #ifdef BUILD_PICOTCP
 
  
  uintptr_t s =  elua_listen( port,TRUE );
   if (lua_isfunction(L,2) && s ) {
      register_socket_callback(L,s,2);
   }
  push_socket(L,s);  // return socket 
  #else
  lua_pushinteger (L,( elua_listen( port,TRUE )==0) ?ELUA_NET_ERR_OK:ELUA_NET_ERR_LIMIT_EXCEEDED );
  #endif 
  return 1;
}


//Lua: unlisten(port)
//  will always return ELUA_NET_ERR_OK because there is no check
// if the was active listening in the port.
static int net_unlisten(lua_State *L)
{
#ifdef BUILD_PICOTCP
  //return luaL_error( L, "not implemented with picoTCP" );
  lua_pushinteger(L,ELUA_NET_ERR_OK);
  return 1;
#else
  u16 port = ( u16 )luaL_checkinteger( L, 1 );
  elua_listen( port,FALSE );
  lua_pushinteger(L,ELUA_NET_ERR_OK);
  return 1;
#endif  
}


// Lua: sock, remoteip, err = accept( port, [timer_id, timeout] )
static int net_accept( lua_State *L )
{
  u16 port = ( u16 )luaL_checkinteger( L, 1 );

  // to be compatible with orginal net we just start listening to the port
  // here. The recommended usage is to use listen/unlisten to specifiy
  // listening and use the accept call to take incomming connections the port
#ifndef BUILD_PICOTCP 
    
  int lres=elua_listen(port,TRUE);
   
  if ( lres!=0 )
  { // check if listen was succesfull
    // Remark: With uIP listen is always successfull, even if there are
    // no free listen slots. This is a limitation of the library
    // Nevertheless I kept the error handler for completness.
    // if not return with error
    lua_pushinteger(L,-1); // sock
    lua_pushinteger(L,0); // rem ip
    lua_pushinteger(L,ELUA_NET_ERR_LIMIT_EXCEEDED);
    return 3;
  }
#endif

  unsigned timer_id = PLATFORM_TIMER_SYS_ID;
  timer_data_type timeout = PLATFORM_TIMER_INF_TIMEOUT;
  elua_net_ip remip;
 

  cmn_get_timeout_data( L, 2, &timer_id, &timeout );
 
  sock_t *sock  = push_socket(L, elua_accept( port, timer_id, timeout, &remip ));

  lua_pushinteger( L, ( sock ) ? remip.ipaddr:0 );
  lua_pushinteger( L, ( sock )? ELUA_NET_ERR_OK:ELUA_NET_ERR_WAIT_TIMEDOUT );
  return 3;
}

// Lua: sock = socket( type )
static int net_socket( lua_State *L )
{
  int type = ( int )luaL_checkinteger( L, 1 );

  push_socket( L, elua_net_socket( type ));
  return 1;
}



// Lua: res = close( socket )
static int net_close( lua_State* L )
{
  sock_t *s = get_socket(L,1);
  if ( s  ) {
    lua_pushinteger( L, elua_net_close( s->sock ) );
    s->sock=0;
    return 1;
  } else
   return 0;
}

// Lua: res, err = send( sock, str )
static int net_send( lua_State* L )
{
  
  sock_t *s = get_socket(L,1);
  if ( s==NULL  ) return 0;
  
  const char *buf;
  size_t len;

  luaL_checktype( L, 2, LUA_TSTRING );
  buf = lua_tolstring( L, 2, &len );
  lua_pushinteger( L, elua_net_send( s->sock, buf, len ) );
  lua_pushinteger( L, elua_net_get_last_err( s->sock ) );
  return 2;
}

// Lua: err = connect( sock, iptype, port )
// "iptype" is actually an int returned by "net.packip"
static int net_connect( lua_State *L )
{
  elua_net_ip ip;
  sock_t *s = get_socket(L,1);
  if (s==NULL) return 0; 

  u16 port = ( int )luaL_checkinteger( L, 3 );

  ip.ipaddr = ( u32 )luaL_checkinteger( L, 2 );
  elua_net_connect( s->sock, ip, port );
  lua_pushinteger( L, elua_net_get_last_err( s->sock ) );
  return 1;
}

// Lua: data = packip( ip0, ip1, ip2, ip3 ), or
// Lua: data = packip( "ip" )
// Returns an internal representation for the given IP address
static int net_packip( lua_State *L )
{
  elua_net_ip ip;
  unsigned i, temp;

  if( lua_isnumber( L, 1 ) )
    for( i = 0; i < 4; i ++ )
    {
      temp = luaL_checkinteger( L, i + 1 );
      if( temp < 0 || temp > 255 )
        return luaL_error( L, "invalid IP adddress" );
      ip.ipbytes[ i ] = temp;
    }
  else
  {
    const char* pip = luaL_checkstring( L, 1 );
    unsigned len, temp[ 4 ];

    if( sscanf( pip, "%u.%u.%u.%u%n", temp, temp + 1, temp + 2, temp + 3, &len ) != 4 || len != strlen( pip ) )
      return luaL_error( L, "invalid IP adddress" );
    for( i = 0; i < 4; i ++ )
    {
      if( temp[ i ] < 0 || temp[ i ] > 255 )
        return luaL_error( L, "invalid IP address" );
      ip.ipbytes[ i ] = ( u8 )temp[ i ];
    }
  }
  lua_pushinteger( L, ip.ipaddr );
  return 1;
}

// Lua: ip0, ip1, ip2, ip3 = unpackip( iptype, "*n" ), or
//               string_ip = unpackip( iptype, "*s" )
static int net_unpackip( lua_State *L )
{
  elua_net_ip ip;
  unsigned i;
  const char* fmt;

  ip.ipaddr = ( u32 )luaL_checkinteger( L, 1 );
  fmt = luaL_checkstring( L, 2 );
  if( !strcmp( fmt, "*n" ) )
  {
    for( i = 0; i < 4; i ++ )
      lua_pushinteger( L, ip.ipbytes[ i ] );
    return 4;
  }
  else if( !strcmp( fmt, "*s" ) )
  {
    lua_pushfstring( L, "%d.%d.%d.%d", ( int )ip.ipbytes[ 0 ], ( int )ip.ipbytes[ 1 ],
                     ( int )ip.ipbytes[ 2 ], ( int )ip.ipbytes[ 3 ] );
    return 1;
  }
  else
    return luaL_error( L, "invalid format" );
}

// Lua: res, err = recv( sock, maxsize, [timer_id, timeout] ) or
//      res, err = recv( sock, "*l", [timer_id, timeout] )
static int net_recv( lua_State *L )
{
  sock_t *s = get_socket(L,1);
  if ( s==NULL ) return 0;

  elua_net_size maxsize;
  s16 lastchar = ELUA_NET_NO_LASTCHAR;
  unsigned timer_id = PLATFORM_TIMER_SYS_ID;
  timer_data_type timeout = PLATFORM_TIMER_INF_TIMEOUT;
  luaL_Buffer net_recv_buff;

  if( lua_isnumber( L, 2 ) ) // invocation with maxsize
    maxsize = ( elua_net_size )luaL_checkinteger( L, 2 );
  else // invocation with line mode
  {
    if( strcmp( luaL_checkstring( L, 2 ), "*l" ) )
      return luaL_error( L, "invalid second argument to recv" );
    lastchar = '\n';
    maxsize = BUFSIZ;
  }
  cmn_get_timeout_data( L, 3, &timer_id, &timeout );
  // Initialize buffer
  luaL_buffinit( L, &net_recv_buff );
  int size=elua_net_recvbuf( s->sock, &net_recv_buff, maxsize, lastchar, timer_id, timeout );
  luaL_pushresult( &net_recv_buff );
  lua_pushinteger( L, size>=0?ELUA_NET_ERR_OK:ELUA_NET_ERR_TIMEDOUT );
  return 2;
}

// Lua: iptype = lookup( "name" )
static int net_lookup( lua_State* L )
{
  const char* name = luaL_checkstring( L, 1 );
  elua_net_ip res;

  res = elua_net_lookup( name );
  lua_pushinteger( L, res.ipaddr );
  return 1;
}




static int net_stack_tick( lua_State* L )
{
#ifdef BUILD_PICOTCP  
  elua_pico_tick();
#endif   
  return 0; 

}


static int socket_gc( lua_State *L )
{
  sock_t *s = sock_check( L, 1 );
  
  if( s && s->sock != 0 )
  {
    elua_net_close( s->sock );
    s->sock = 0;
  }
  return 0; 
}


#ifdef BUILD_PICOTCP

static const char * sock_events[] = {
  "connect", 
  "read",
  "write",
  "close",
  "fin",
  "error"
};

static void socket_callback(t_socket_event ev, uintptr_t socket)
{
  
  if ( !G_state ) return;

  sock_t *s = map_socket( G_state,socket );
 
  if ( s ) {
    lua_getfield( s->L,LUA_REGISTRYINDEX,NET_CALLBACK_T_NAME );
    lua_pushlightuserdata( s->L,(void*)socket );
    lua_gettable( s->L,-2 ); // Try to find callback
    lua_remove( s->L,-2 );  // remove callback table from stack
    if ( lua_isfunction(s->L,-1) ) {
      lua_pushstring( s->L,sock_events[(int)ev] );
      lua_pushvalue( G_state,-1 ); // Dup socket object
      lua_xmove( G_state,s->L,1 ); 
      if ( lua_pcall( s->L,2,0,0 ) != 0 ) {
        elua_pico_unwind(); 
        lua_error( s->L );
      };
   } else {
     lua_pop(s->L,1); // clean stack 
   }
   switch ( ev ) {
      case ELUA_NET_FIN:
        //printk("Socket FIN event\n");
        if ( s ) s->sock=0;
        break; 

      default: 
        ;// nothing
    }
  }

  lua_pop( G_state,1 ); // clean Lua stack 
}


static int net_socket_callback(lua_State *L )
{
  sock_t *s = sock_check( L, 1 );

 
  if ( lua_isfunction(s->L,2)  ) {
     register_socket_callback(L,s->sock,2);
   } else
   {
     luaL_typerror(L,2,"function type");
   }
   return 0;
}

#endif 


// Module function map


#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE net_map[] =
{
  { LSTRKEY( "accept" ), LFUNCVAL( net_accept ) },
  { LSTRKEY( "packip" ), LFUNCVAL( net_packip ) },
  { LSTRKEY( "unpackip" ), LFUNCVAL( net_unpackip ) },
  { LSTRKEY( "connect" ), LFUNCVAL( net_connect ) },
  { LSTRKEY( "socket" ), LFUNCVAL( net_socket ) },
  { LSTRKEY( "close" ), LFUNCVAL( net_close ) },
  { LSTRKEY( "send" ), LFUNCVAL( net_send ) },
  { LSTRKEY( "recv" ), LFUNCVAL( net_recv ) },
  { LSTRKEY( "lookup" ), LFUNCVAL( net_lookup ) },
  { LSTRKEY( "listen" ), LFUNCVAL( net_listen ) }, // TH
  { LSTRKEY( "unlisten" ), LFUNCVAL( net_unlisten ) }, // TH
#ifdef BUILD_PICOTCP 
  { LSTRKEY( "tick" ), LFUNCVAL( net_stack_tick ) }, // TH
  { LSTRKEY( "socket_table" ), LFUNCVAL( net_get_sockettable ) }, // TH
  { LSTRKEY( "callback" ), LFUNCVAL( net_socket_callback ) }, // TH
  
#endif 
#if LUA_OPTIMIZE_MEMORY > 0 
  { LSTRKEY( "SOCK_STREAM" ), LNUMVAL( ELUA_NET_SOCK_STREAM ) },
  { LSTRKEY( "SOCK_DGRAM" ), LNUMVAL( ELUA_NET_SOCK_DGRAM ) },
  { LSTRKEY( "ERR_OK" ), LNUMVAL( ELUA_NET_ERR_OK ) },
  { LSTRKEY( "ERR_TIMEOUT" ), LNUMVAL( ELUA_NET_ERR_TIMEDOUT ) },
  { LSTRKEY( "ERR_CLOSED" ), LNUMVAL( ELUA_NET_ERR_CLOSED ) },
  { LSTRKEY( "ERR_ABORTED" ), LNUMVAL( ELUA_NET_ERR_ABORTED ) },
  { LSTRKEY( "ERR_OVERFLOW" ), LNUMVAL( ELUA_NET_ERR_OVERFLOW ) },
  { LSTRKEY( "ERR_LIMIT_EXCEEDED" ), LNUMVAL( ELUA_NET_ERR_LIMIT_EXCEEDED ) }, //TH
  { LSTRKEY( "ERR_WAIT_TIMEDOUT" ), LNUMVAL( ELUA_NET_ERR_WAIT_TIMEDOUT ) }, //TH
  { LSTRKEY( "ERR_INVALID_SOCKET" ), LNUMVAL( ELUA_NET_ERR_INVALID_SOCKET ) }, //TH

  { LSTRKEY( "NO_TIMEOUT" ), LNUMVAL( 0 ) },
  { LSTRKEY( "INF_TIMEOUT" ), LNUMVAL( PLATFORM_TIMER_INF_TIMEOUT ) },
#endif
  { LNILKEY, LNILVAL }
};


static const LUA_REG_TYPE socket_mt_map[] = 
{
  { LSTRKEY( "__gc" ), LFUNCVAL( socket_gc ) },
  { LSTRKEY( "__index" ), LRO_ROVAL( socket_mt_map ) },
  { LSTRKEY( "connect" ), LFUNCVAL( net_connect ) },
  { LSTRKEY( "close" ), LFUNCVAL( net_close ) },
  { LSTRKEY( "send" ), LFUNCVAL( net_send ) },
  { LSTRKEY( "recv" ), LFUNCVAL( net_recv ) },
  { LSTRKEY( "callback" ), LFUNCVAL( net_socket_callback ) }, // TH
  { LNILKEY, LNILVAL }
};



LUALIB_API int luaopen_net( lua_State *L )
{

 
#if LUA_OPTIMIZE_MEMORY > 0 
  
  luaL_rometatable( L, NET_META_NAME, ( void* )socket_mt_map );
  
  init_socket_table(L);
#ifdef BUILD_PICOTCP
  elua_pico_set_socketcallback(&socket_callback);
#endif 

  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_NET, net_map );  
  init_socket_table(L);
  // Module constants
  MOD_REG_NUMBER( L, "SOCK_STREAM", ELUA_NET_SOCK_STREAM );
  MOD_REG_NUMBER( L, "SOCK_DGRAM", ELUA_NET_SOCK_DGRAM );
  MOD_REG_NUMBER( L, "ERR_OK", ELUA_NET_ERR_OK );
  MOD_REG_NUMBER( L, "ERR_TIMEDOUT", ELUA_NET_ERR_TIMEDOUT );
  MOD_REG_NUMBER( L, "ERR_CLOSED", ELUA_NET_ERR_CLOSED );
  MOD_REG_NUMBER( L, "ERR_ABORTED", ELUA_NET_ERR_ABORTED );
  MOD_REG_NUMBER( L, "ERR_OVERFLOW", ELUA_NET_ERR_OVERFLOW );
  MOD_REG_NUMBER( L, "ELUA_NET_ERR_INVALID_SOCKET", ELUA_NET_ERR_INVALID_SOCKET );
  MOD_REG_NUMBER( L, "NO_TIMEOUT", 0 );
  MOD_REG_NUMBER( L, "INF_TIMEOUT", PLATFORM_TIMER_INF_TIMEOUT );

  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}

void elua_net_cleanup()
{
   G_state=NULL; 
}

#else // #ifdef BUILD_UIP

LUALIB_API int luaopen_net( lua_State *L )
{
  return 0;
}

void elua_net_cleanup()
{
   
}

#endif // #ifdef BUILD_UIP

