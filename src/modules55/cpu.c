// Lua 5.5 CPU module, initially exposing platform-neutral clock information.

#include "elua_lua55.h"
#include "platform.h"

static int cpu_clock( elua_lua55_State* L )
{
  elua_lua55_pushinteger( L, ( long long )platform_cpu_get_frequency() );
  return 1;
}

int elua_lua55open_cpu( elua_lua55_State* L )
{
  elua_lua55_newtable( L );
  elua_lua55_pushcfunction( L, cpu_clock );
  elua_lua55_setfield( L, -2, "clock" );
  return 1;
}
