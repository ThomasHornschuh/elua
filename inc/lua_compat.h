// Compatibility helpers for building existing eLua modules with Lua 5.5.

#ifndef ELUA_LUA_COMPAT_H
#define ELUA_LUA_COMPAT_H

#if defined(ELUA_LUA55)

#include "lua.h"
#include "lauxlib.h"

typedef luaL_Reg luaL_reg;

static void elua_luaL_register( lua_State* L, const char* name, const luaL_Reg* functions )
{
  lua_createtable( L, 0, 0 );
  luaL_setfuncs( L, functions, 0 );
  if( name != NULL )
  {
    lua_pushvalue( L, -1 );
    lua_setglobal( L, name );
  }
}

#define luaL_register elua_luaL_register
#ifndef lua_objlen
#define lua_objlen lua_rawlen
#endif
#define luaL_typerror luaL_typeerror
#ifndef luaL_putchar
#define luaL_putchar luaL_addchar
#endif
#ifndef lua_isrotable
#define lua_isrotable(L, index) 0
#endif

#endif // defined(ELUA_LUA55)

#endif // ELUA_LUA_COMPAT_H
