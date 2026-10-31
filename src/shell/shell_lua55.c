// Shell: standalone Lua 5.5 interpreter implementation.
//
// Lua 5.5 is deliberately included as a single translation unit.  Its API
// symbols are private to this file, allowing it to coexist with eLua's Lua
// 5.1 interpreter until eLua's Lua-specific libraries are ported.

#include <stdio.h>

#include "shell.h"

#define ELUA_LUA55
#define LUA_PROGNAME "lua55"
#define main lua55_main
#include "../lua55/onelua.c"
#undef main

const char shell_help_lua55[] = "[options] [script [args]]\n"
  "  start the standalone Lua 5.5 interpreter (without eLua modules).\n";
const char shell_help_summary_lua55[] = "start a Lua 5.5 session";

void shell_lua55( int argc, char **argv )
{
  printf( "Starting Lua 5.5 (without eLua modules)\n" );
  lua55_main( argc, argv );
  clearerr( stdin );
}
