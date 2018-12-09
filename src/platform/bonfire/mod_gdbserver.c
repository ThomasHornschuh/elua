


#include "platform_conf.h"
#include "lauxlib.h"
#include "lrotable.h"
#include <string.h>
#include "encoding.h"

#include "gdb_interface.h"
#include "console.h"


static int debug_initalized=0;

int initgdbserver( lua_State* L )
{
  unsigned debug_port= ( unsigned )luaL_optinteger( L, 1, GDB_DEBUG_UART );  
  unsigned baudrate = ( unsigned )luaL_optinteger( L, 2, GDB_DEBUG_BAUD );  
  MOD_CHECK_ID( uart, debug_port );
  
  unsigned res = gdb_setup_interface(debug_port,baudrate);
  if (res) {
    gdb_debug_handler=gdb_initDebugger(0);
    printk("Connect with Debugger port with %d baud\n",baudrate);
    debug_initalized=1;
    gdb_breakpoint();
    lua_pushinteger( L, res );
    return 1;
  } else {
    return luaL_error( L, "gdbserver.init called with invalid parameters" );
  }  
}


static int debugenable( lua_State* L )
{
   if (debug_initalized) {
     gdb_enable_debugger();
     return 1;
   } else
      return luaL_error( L, "gdbserver not initalized" );
}


static int debugdisable( lua_State* L )
{
   if (debug_initalized) {
     gdb_disable_debugger();
     return 1;
   } else
      return luaL_error( L, "gdbserver not initalized" );
}


static int debugger( lua_State* L )
{
   if (debug_initalized) {
     gdb_breakpoint();
     return 1;
   } else
      return luaL_error( L, "gdbserver not initalized" );
}



// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE gdbserver_map[] =
{
  { LSTRKEY( "init" ), LFUNCVAL( initgdbserver ) },
  { LSTRKEY( "enable" ), LFUNCVAL( debugenable ) },
  { LSTRKEY( "disable" ), LFUNCVAL( debugdisable ) },
  { LSTRKEY( "debugger" ), LFUNCVAL( debugger ) },


  { LNILKEY, LNILVAL }
};

//LUALIB_API int luaopen_gdbserver(lua_State *L)
//{
  //LREGISTER(L,"gdbserver",gdbserver_map);
//}
