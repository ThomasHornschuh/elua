// Shell: standalone Lua 5.5 interpreter implementation.
//
// Lua 5.5 is deliberately included as a single translation unit.  Its API
// symbols are private to this file, allowing it to coexist with eLua's Lua
// 5.1 interpreter until eLua's Lua-specific libraries are ported.

#include <stdio.h>

#include "shell.h"
#include "elua_lua55.h"

#define ELUA_LUA55
#define LUA_PROGNAME "lua55"
#define main lua55_main
#include "../lua55/onelua.c"
#undef main

void elua_lua55_newtable( elua_lua55_State* L )
{
  lua_newtable( L );
}

void elua_lua55_pushcfunction( elua_lua55_State* L, elua_lua55_CFunction f )
{
  lua_pushcfunction( L, ( lua_CFunction )f );
}

void elua_lua55_pushstring( elua_lua55_State* L, const char* value )
{
  lua_pushstring( L, value );
}

void elua_lua55_pushinteger( elua_lua55_State* L, long long value )
{
  lua_pushinteger( L, ( lua_Integer )value );
}

void elua_lua55_setfield( elua_lua55_State* L, int index, const char* key )
{
  lua_setfield( L, index, key );
}

const char* elua_lua55_checkstring( elua_lua55_State* L, int index )
{
  return luaL_checkstring( L, index );
}

int elua_lua55_error( elua_lua55_State* L, const char* message )
{
  return luaL_error( L, "%s", message );
}

const char shell_help_lua55[] = "[options] [script [args]]\n"
  "  start Lua 5.5 (with the eLua pd, elua and cpu modules).\n";
const char shell_help_summary_lua55[] = "start a Lua 5.5 session";

void shell_lua55( int argc, char **argv )
{
  printf( "Starting Lua 5.5 (with eLua pd, elua and cpu modules)\n" );
  lua55_main( argc, argv );
  clearerr( stdin );
}
