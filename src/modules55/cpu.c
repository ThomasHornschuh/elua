// Lua 5.5 CPU module, initially exposing platform-neutral clock information.

#include "lua.h"
#include "lauxlib.h"
#include "platform.h"

static int cpu_clock( lua_State* L )
{
  lua_pushinteger( L, ( lua_Integer )platform_cpu_get_frequency() );
  return 1;
}

int luaopen_cpu( lua_State* L )
{
  lua_newtable( L );
  lua_pushcfunction( L, cpu_clock );
  lua_setfield( L, -2, "clock" );
  return 1;
}
