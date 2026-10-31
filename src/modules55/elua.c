// Lua 5.5 subset of eLua's core-services module.

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "elua_lua55.h"
#include "shell.h"

#if defined( USE_GIT_REVISION )
#include "git_version.h"
#else
#include "version.h"
#endif

static int elua_version( elua_lua55_State* L )
{
  elua_lua55_pushstring( L, ELUA_STR_VERSION );
  return 1;
}

static int elua_heapstats( elua_lua55_State* L )
{
#ifndef USE_SIMPLE_ALLOCATOR
  struct mallinfo m = mallinfo();
  elua_lua55_pushinteger( L, ( long long )m.arena );
  elua_lua55_pushinteger( L, ( long long )m.uordblks );
  return 2;
#else
  ( void )L;
  return 0;
#endif
}

#ifdef BUILD_SHELL
static int elua_shell( elua_lua55_State* L )
{
  const char* command = elua_lua55_checkstring( L, 1 );
  char* command_copy = malloc( strlen( command ) + 2 );

  if( command_copy == NULL )
    return elua_lua55_error( L, "not enough memory for elua.shell" );
  strcpy( command_copy, command );
  shellh_execute_command( command_copy, 0 );
  free( command_copy );
  return 0;
}
#endif

int elua_lua55open_elua( elua_lua55_State* L )
{
  elua_lua55_newtable( L );

  elua_lua55_pushcfunction( L, elua_version );
  elua_lua55_setfield( L, -2, "version" );
  elua_lua55_pushcfunction( L, elua_heapstats );
  elua_lua55_setfield( L, -2, "heapstats" );
#ifdef BUILD_SHELL
  elua_lua55_pushcfunction( L, elua_shell );
  elua_lua55_setfield( L, -2, "shell" );
#endif
  return 1;
}
