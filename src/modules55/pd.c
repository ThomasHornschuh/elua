// Lua 5.5 platform-data module.

#include "elua_lua55.h"

#define MACRO_NAME( x ) MACRO_AGAIN( x )
#define MACRO_AGAIN( x ) #x

static int pd_platform( elua_lua55_State* L )
{
  elua_lua55_pushstring( L, MACRO_NAME( ELUA_PLATFORM ) );
  return 1;
}

static int pd_cpu( elua_lua55_State* L )
{
  elua_lua55_pushstring( L, MACRO_NAME( ELUA_CPU ) );
  return 1;
}

static int pd_board( elua_lua55_State* L )
{
  elua_lua55_pushstring( L, MACRO_NAME( ELUA_BOARD ) );
  return 1;
}

int elua_lua55open_pd( elua_lua55_State* L )
{
  elua_lua55_newtable( L );

  elua_lua55_pushcfunction( L, pd_platform );
  elua_lua55_setfield( L, -2, "platform" );
  elua_lua55_pushcfunction( L, pd_cpu );
  elua_lua55_setfield( L, -2, "cpu" );
  elua_lua55_pushcfunction( L, pd_board );
  elua_lua55_setfield( L, -2, "board" );
  return 1;
}
