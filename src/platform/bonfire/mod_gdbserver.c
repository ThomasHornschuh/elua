#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "platform_conf.h"
#include "lrotable.h"
#include <string.h>
#include "encoding.h"

#include "riscv-gdb-stub.h"
#include "console.h"

extern t_ptrapfuntion gdb_debug_handler;


#define DEBUG_BAUD 115200

static int debug_initalized=0;

static int initgdbserver( lua_State* L )
{
  gdb_setup_interface(DEBUG_BAUD);
  gdb_debug_handler=gdb_initDebugger(0);
  printk("Connect with Debugger port with %d baud\n",DEBUG_BAUD);
  debug_initalized=1;
  gdb_breakpoint();
  return 1;
}


static int debugenable( lua_State* L )
{
   if (debug_initalized) {
     gdb_debug_handler=gdb_initDebugger(0);
     return 1;
   } else
      return luaL_error( L, "gdbserver not initalized" );
}


static int debugdisable( lua_State* L )
{
   if (debug_initalized) {
     gdb_debug_handler=NULL;
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
