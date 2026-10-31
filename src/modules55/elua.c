// Lua 5.5 subset of eLua's core-services module.

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "shell.h"

#if defined( USE_GIT_REVISION )
#include "git_version.h"
#else
#include "version.h"
#endif

static int elua_version( lua_State* L )
{
  lua_pushstring( L, ELUA_STR_VERSION );
  return 1;
}

static int elua_heapstats( lua_State* L )
{
#ifndef USE_SIMPLE_ALLOCATOR
  struct mallinfo m = mallinfo();
  lua_pushinteger( L, ( lua_Integer )m.arena );
  lua_pushinteger( L, ( lua_Integer )m.uordblks );
  return 2;
#else
  ( void )L;
  return 0;
#endif
}

#ifdef BUILD_SHELL
static int elua_shell( lua_State* L )
{
  const char* command = luaL_checkstring( L, 1 );
  char* command_copy = malloc( strlen( command ) + 2 );

  if( command_copy == NULL )
    return luaL_error( L, "not enough memory for elua.shell" );
  strcpy( command_copy, command );
  shellh_execute_command( command_copy, 0 );
  free( command_copy );
  return 0;
}
#endif

int luaopen_elua( lua_State* L )
{
  lua_newtable( L );

  lua_pushcfunction( L, elua_version );
  lua_setfield( L, -2, "version" );
  lua_pushcfunction( L, elua_heapstats );
  lua_setfield( L, -2, "heapstats" );
#ifdef BUILD_SHELL
  lua_pushcfunction( L, elua_shell );
  lua_setfield( L, -2, "shell" );
#endif
  return 1;
}
