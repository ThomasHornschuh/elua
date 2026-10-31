// Lua 5.5 platform-data module.

#include "lua.h"
#include "lauxlib.h"

#define MACRO_NAME( x ) MACRO_AGAIN( x )
#define MACRO_AGAIN( x ) #x

static int pd_platform( lua_State* L )
{
  lua_pushstring( L, MACRO_NAME( ELUA_PLATFORM ) );
  return 1;
}

static int pd_cpu( lua_State* L )
{
  lua_pushstring( L, MACRO_NAME( ELUA_CPU ) );
  return 1;
}

static int pd_board( lua_State* L )
{
  lua_pushstring( L, MACRO_NAME( ELUA_BOARD ) );
  return 1;
}

int luaopen_pd( lua_State* L )
{
  lua_newtable( L );

  lua_pushcfunction( L, pd_platform );
  lua_setfield( L, -2, "platform" );
  lua_pushcfunction( L, pd_cpu );
  lua_setfield( L, -2, "cpu" );
  lua_pushcfunction( L, pd_board );
  lua_setfield( L, -2, "board" );
  return 1;
}
