/*
** $Id: linit.c $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB


#include "lprefix.h"


#include <stddef.h>

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"
#include "llimits.h"

#if defined(ELUA_LUA55)
#include "platform_conf.h"

static int luaopen_dummy (lua_State *L) {
  (void)L;
  return 0;
}

#if defined(PLATFORM_MODULES_ENABLE)
/* Lua 5.1 built the platform namespace in its customised linit.c.  Keep the
 * same `bonfire.<module>` API when the selected runtime is Lua 5.5. */
typedef struct {
  const char *name;
  const luaL_Reg *functions;
} elua_platform_module;

#undef _ROM
#define _ROM( name, openf, table ) extern const luaL_Reg table[];
PLATFORM_MODULES_LIBS_ROM

static const elua_platform_module platform_modules[] = {
#undef _ROM
#define _ROM( name, openf, table ) { name, table },
  PLATFORM_MODULES_LIBS_ROM
  {NULL, NULL}
};

LUALIB_API int luaopen_platform (lua_State *L) {
  const elua_platform_module *module;

  lua_newtable(L);
  for (module = platform_modules; module->name != NULL; module++) {
    lua_createtable(L, 0, 0);
    luaL_setfuncs(L, module->functions, 0);
    lua_setfield(L, -2, module->name);
  }
  return 1;
}
#endif

#undef _ROM
#define _ROM( name, openf, table ) { name, openf },
static const luaL_Reg eluamods[] = {
#if defined(LUA_PLATFORM_LIBS_ROM)
  LUA_PLATFORM_LIBS_ROM
#endif
  {NULL, NULL}
};
#endif


/*
** Standard Libraries. (Must be listed in the same ORDER of their
** respective constants LUA_<libname>K.)
*/
static const luaL_Reg stdlibs[] = {
  {LUA_GNAME, luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_DBLIBNAME, luaopen_debug},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {NULL, NULL}
};


/*
** require and preload selected standard libraries
*/
LUALIB_API void luaL_openselectedlibs (lua_State *L, int load, int preload) {
  int mask;
  const luaL_Reg *lib;
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  for (lib = stdlibs, mask = 1; lib->name != NULL; lib++, mask <<= 1) {
    if (load & mask) {  /* selected? */
      luaL_requiref(L, lib->name, lib->func, 1);  /* require library */
      lua_pop(L, 1);  /* remove result from the stack */
    }
    else if (preload & mask) {  /* selected? */
      lua_pushcfunction(L, lib->func);
      lua_setfield(L, -2, lib->name);  /* add library to PRELOAD table */
    }
  }
  lua_assert((mask >> 1) == LUA_UTF8LIBK);
  lua_pop(L, 1);  /* remove PRELOAD table */
#if defined(ELUA_LUA55)
  for (lib = eluamods; lib->name != NULL; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);
  }
#endif
}
