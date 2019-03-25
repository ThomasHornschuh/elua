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
#include <errno.h>

#include "console.h" // TODO: Remove...

#include "platform_conf.h"
#ifdef BUILD_TCPIP

#ifdef BUILD_PICOTCP
#include "pico_socket.h"
#include "pico_stack.h"
#include "elua_picotcp.h"
#endif

#define NET_META_NAME      "eLua.net.socket"
#define NET_SOCK_T_NAME    "eLua.net.socketTable"
#define NET_CALLBACK_T_NAME "eLua.net.callbackTable"
#define NET_WEAK_META_NAME  "eLua.net.weakmetaTable"
#define NET_TIMER_META_NAME "elua.net.timer"
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



// (UN)Registers a callback function with keyID. KeyID should be a unique ptr
// Expects  a function or nil at Stack position idx (idx must be an absolute - positive - index)
// If nil is passed on the stack position the entry is effecilvy deleted

static void register_callback( lua_State *L,void*  keyId, int idx )
{

 //  int top=lua_gettop( L );
   if (!(lua_isfunction( L,idx ) || lua_isnil(L, idx )) ) {
     luaL_error(L,"function or nil expected at index %d",idx);
   }
   lua_getfield(L,LUA_REGISTRYINDEX,NET_CALLBACK_T_NAME);
   lua_pushlightuserdata(L,(void*)keyId);
   lua_pushvalue(L,idx);
   lua_settable(L,-3);
   lua_pop(L,1);
//   if (top!=lua_gettop(L)) kassert_fail("register_callback");
}


// Pushes the function associated with keyId to the top of the stack
static void push_callback( lua_State *L, void* keyId )
{
  lua_getfield( L,LUA_REGISTRYINDEX,NET_CALLBACK_T_NAME );
  lua_pushlightuserdata( L,(void*)keyId );
  lua_gettable( L,-2 ); // Try to find callback
  lua_remove( L,-2 );  // remove callback table from stack
}

static void unref_callback( lua_State *L, void* keyId )
{
  lua_pushnil( L );
  register_callback( L, keyId, lua_gettop( L ) ); 
  lua_pop( L, 1 );
  
}

// Helper function for cleaning up a socket
static void finalize_socket( lua_State *L, sock_t *s)
{
  if ( s->sock ) {
    lua_pushnil( L );
    register_callback( L,(void*) s->sock, lua_gettop( L ) );
    lua_pop( L, 1 );
    s->sock=0;
  }

}

// Creates new userdata and assigns the  metatable registered with name meta_name to it 
static void* newuserdata_meta( lua_State *L, size_t sz, const char * meta_name )
{
void * ud = lua_newuserdata( L , sz );

  luaL_getmetatable( L, meta_name );
  lua_setmetatable( L, -2 );

  return ud; 
}

static sock_t *push_socket( lua_State *L, uintptr_t sock )
{
    if (stack_s_valid(sock)) {
      // Create userdata object for socket
      sock_t  *s = ( sock_t* )newuserdata_meta( L, sizeof( sock_t ), NET_META_NAME );
      s->sock = sock;
      
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

static void callback_error( lua_State *L )
{
const char * err = lua_tostring( L, -1 );

  if (err==NULL) err = lua_typename( L, -1 );

  printf( "error while processing network callback: %s\n",err );

  lua_pop( L,1 );
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


  uintptr_t s =  elua_listen( port, TRUE );
   if (lua_isfunction(L,2) && s ) {
      register_callback( L, (void*) s, 2 );
   }
  push_socket( L, s );  // return socket
  #else
  lua_pushinteger ( L,( elua_listen( port,TRUE )==0 ) ?ELUA_NET_ERR_OK:ELUA_NET_ERR_LIMIT_EXCEEDED );
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



#ifdef BUILD_PICOTCP
// Lua: iptype = lookup( "name" ) or
// Lua lookup (name, function(iptype))

static void cb_dns( dns_result_t * r )
{
  if (G_state == NULL) return ; // Don't run callbacks when Lua is not initalized

   
   lua_State *L = lua_newthread( G_state );
   if (!L) kassert_fail("dns_cb L is NULL");


  push_callback( L, (void*)r );
  if ( lua_isfunction(L,-1) ) {
    unref_callback( L, (void*)r );
    //  Do the callback
    lua_pushinteger( L,  r->ipaddr );
    int res = lua_resume( L, 1 );
    if (  res != 0 && res !=LUA_YIELD ) {
        callback_error( L );
    }
  } 
  lua_pop( G_state, 1 ); // will hopefully GC the thread 

  free( r );
}


static int net_lookup( lua_State* L )
{
  const char* name = luaL_checkstring( L, 1 );


  if ( lua_isfunction( L, 2) ) {
    // Run in callback mode
    dns_result_t * r = elua_net_lookup_async( name, &cb_dns );
    if ( r ) {
      register_callback( L , r, 2 );
    } else     {
      luaL_error(L, "net.lookup failed");

    }
    return 0;
  } else { // sync mode
    elua_net_ip res = elua_net_lookup( name );
    lua_pushinteger( L, res.ipaddr );
    return 1;
  }

}


// Timers repurpose the sock_t structure to store a timer id instead of sockets
// Besides a different metatable there is not other difference 


typedef struct {
  uint32_t id; // Timer Id
  int ref; // Registry Index of associated callback

} t_net_timer;

#define net_timer_check( L, idx ) ( t_net_timer* )luaL_checkudata( L, idx, NET_TIMER_META_NAME )

static void cb_timer(pico_time time, void * arg)
{
   if (G_state == NULL) return ; // Don't run callbacks when Lua is not initalized
   
   lua_State *L = lua_newthread( G_state );
   if (!L) kassert_fail("cb_timer: L is NULL");

   lua_rawgeti(L, LUA_REGISTRYINDEX,(int) arg );
   if ( lua_isfunction( L, -1) ) {
     
     // Do the callback
     lua_pushnumber( L, time );
     int res = lua_resume( L, 1 );
     if (  res != 0 && res !=LUA_YIELD ) {
        callback_error( L );
     }
   }
   luaL_unref(L, LUA_REGISTRYINDEX, (int)arg );
   lua_pop( G_state, 1 ); // will hopefully GC the thread 
   
}


static int net_timer( lua_State *L )
{

   pico_time  expire = (pico_time)luaL_checknumber (L, 1);
   luaL_checkanyfunction( L, 2);

   t_net_timer * t = (t_net_timer*) newuserdata_meta( L, sizeof(t_net_timer), NET_TIMER_META_NAME ); 
   // Store callback function in registry
   lua_pushvalue( L, 2 );  
   t->ref = luaL_ref(L, LUA_REGISTRYINDEX);
   t->id = pico_timer_add(  expire, &cb_timer, (void*) t->ref );

   return 1; // return userdata object

}

static int net_timer_cancel( lua_State *L )
{

t_net_timer *t = net_timer_check( L, 1 );
bool valid=false;
  
   if (t->id) {
     // Check if calllback is still registered - this means that the timer has not expired yet 
     lua_rawgeti(L, LUA_REGISTRYINDEX,t->ref );
     valid = lua_isfunction (L, -1 );
     if (valid) {
        pico_timer_cancel( t->id );
        luaL_unref(L, LUA_REGISTRYINDEX, t->ref );
        t->id=0;
        t->ref=0;
     }
     lua_pop( L, 1 ); 
   }  
   lua_pushboolean( L, valid );
   return 1;
}

static int net_timer_to_string( lua_State *L )
{
t_net_timer *t = net_timer_check( L, 1 );

   lua_pushfstring( L, "net.timer id(%d)",t->id );
   return 1;
}

#else
// Lua: iptype = lookup( "name" )
static int net_lookup( lua_State* L )
{
  const char* name = luaL_checkstring( L, 1 );
  elua_net_ip res;

  res = elua_net_lookup( name );
  lua_pushinteger( L, res.ipaddr );
  return 1;
}
#endif




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
    finalize_socket( L, s );
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

static void cb_socket(t_socket_event ev, uintptr_t socket)
{

  if ( !G_state ) return;

  sock_t *s = map_socket( G_state,socket );

  if ( s ) {
    lua_State *L = lua_newthread( G_state );
    push_callback( L, (void*)socket );
    if ( lua_isfunction( L,-1) ) {
      lua_pushstring( L ,sock_events[(int)ev] );
      lua_pushvalue( G_state,-2 ); // Dup socket object
      lua_xmove( G_state, L, 1 );
      int res= lua_resume( L, 2 );
      if ( res!=0 && res != LUA_YIELD ) {
        callback_error( L );
      }; 
   }
   lua_pop( G_state, 1 ); // pop thread  
   switch ( ev ) {
      case ELUA_NET_FIN:
        //printk("Socket FIN event\n");
        if ( s ) finalize_socket( G_state, s);
        break;

      default:
        ;// nothing
    }
  }
  lua_pop( G_state, 1 ); // pop socket
  
}


static int net_socket_callback(lua_State *L )
{
  sock_t *s = sock_check( L, 1 );


  if ( lua_isfunction( L,2 ) || lua_isnil( L, 2 ) ) {
     register_callback( L, (void*)s->sock, 2 );
   } else
   {
     luaL_typerror( L, 2,"function type or nil");
   }
   return 0;
}

static int get_socket_option(lua_State *L)
{
sock_t *s = sock_check( L, 1 );
int option = luaL_checkinteger( L, 2 );


  lua_pushinteger( L, elua_pico_getsocketoption( s->sock,option ));
  return 1;
}

static int set_socket_option(lua_State *L)
{
sock_t *s = sock_check( L, 1 );
int option = luaL_checkinteger( L, 2 );
int optvalue =   luaL_checkinteger( L, 3 );

  lua_pushinteger( L , elua_pico_setsocketoption( s->sock, option, optvalue ));
  return 1;
}

static int net_set_debug_mode( lua_State *L )
{

const char* filename=NULL;  
int mode=luaL_checkint(L, 1 );

  if ( lua_gettop( L )>=2 )
    filename=luaL_checkstring( L,2 ); 

  int err=elua_pico_setdebug_output(filename);
  if (err) {
    return luaL_error( L, strerror(err) );
  } 
  elua_pico_setdebug( mode );
  return 0;

}

static int net_set_nameserver(lua_State *L )
{
elua_net_ip ip;
const char * opmode;
uint8_t flag;

  ip.ipaddr = ( u32 )luaL_checkinteger( L, 1);

  opmode = luaL_checkstring(L, 2 );
  if (strcmp(opmode,"add")==0)
    flag=1;
  else if (strcmp(opmode,"delete")==0)
    flag=0;
  else
    return luaL_error( L, "invalid second arg to net.nameserver, expect add or delete\n");

  lua_pushinteger( L, elua_pico_change_nameserver( ip,flag ) );
  return 1;
}


static int net_getlocal_ip( lua_State *L )
{
elua_net_ip res;

  res = elua_net_get_localip();
  lua_pushinteger( L, res.ipaddr );
  return 1;
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
  { LSTRKEY( "debug" ), LFUNCVAL( net_set_debug_mode ) }, // TH
  { LSTRKEY( "nameserver" ), LFUNCVAL( net_set_nameserver ) }, // TH
  { LSTRKEY( "local_ip" ), LFUNCVAL( net_getlocal_ip ) }, // TH
  { LSTRKEY( "timer" ), LFUNCVAL( net_timer ) }, // TH

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

#ifdef BUILD_PICOTCP
   { LSTRKEY( "TCP_NODELAY" ), LNUMVAL( PICO_TCP_NODELAY ) },
   { LSTRKEY( "OPT_KEEPIDLE" ), LNUMVAL( PICO_SOCKET_OPT_KEEPIDLE ) },
   { LSTRKEY( "OPT_KEEPINTVL" ), LNUMVAL( PICO_SOCKET_OPT_KEEPINTVL ) },
   { LSTRKEY( "OPT_RCVBUF" ), LNUMVAL( PICO_SOCKET_OPT_RCVBUF ) },
   { LSTRKEY( "OPT_SNDBUF" ), LNUMVAL( PICO_SOCKET_OPT_SNDBUF ) },

#endif

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
#ifdef BUILD_PICOTCP
  { LSTRKEY( "callback" ), LFUNCVAL( net_socket_callback ) },
  { LSTRKEY( "getoption" ), LFUNCVAL( get_socket_option ) },
  { LSTRKEY( "setoption" ), LFUNCVAL( set_socket_option ) },
#endif
  { LNILKEY, LNILVAL }
};

#ifdef BUILD_PICOTCP
static const LUA_REG_TYPE timer_mt_map[] =
{
   { LSTRKEY( "__index" ), LRO_ROVAL( timer_mt_map ) },
   //{ LSTRKEY( "__gc" ), LFUNCVAL( timer_gc ) },
   { LSTRKEY( "cancel" ), LFUNCVAL( net_timer_cancel ) },
   { LSTRKEY( "__tostring" ), LFUNCVAL( net_timer_to_string )},
   { LNILKEY, LNILVAL }
};

#endif 


LUALIB_API int luaopen_net( lua_State *L )
{


#if LUA_OPTIMIZE_MEMORY > 0

  luaL_rometatable( L, NET_META_NAME, ( void* )socket_mt_map );

  init_socket_table(L);
#ifdef BUILD_PICOTCP
  elua_pico_set_socketcallback(&cb_socket);
  luaL_rometatable( L, NET_TIMER_META_NAME, ( void* )timer_mt_map );
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

