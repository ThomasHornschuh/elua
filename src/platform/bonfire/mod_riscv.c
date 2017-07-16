#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "platform_conf.h"
#include "lrotable.h"
#include <string.h>
#include "encoding.h"



static int riscv_readcsr( lua_State* L )
{
  unsigned csr = ( unsigned )luaL_checknumber( L, 1 );	
  unsigned r;
  switch(csr) {
	case CSR_MISA: r=read_csr(misa); break;
	case CSR_MIMPID: r=read_csr(mimpid); break;
	case CSR_MSTATUS:  r=read_csr(mstatus); break;
	case CSR_MIE:  r=read_csr(mie); break;
	case CSR_MIP:  r=read_csr(mip); break;
	default: r= -1;
  }	  
  
  lua_pushnumber( L,( lua_Number ) r );
  return 1;
}

extern uint32_t *pmtime; // declared in platform_int.c

static int riscv_readmtime( lua_State* L )
{
	lua_pushnumber( L, (lua_Number)pmtime[0] );
	return 1;
}


static int riscv_readmcycle( lua_State* L )
{
u64 cycle=platform_timer_sys_raw_read();
	
	lua_pushnumber( L, (lua_Number)cycle);
	return 1;
}


// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE riscv_map[] = 
{
  { LSTRKEY( "readcsr" ), LFUNCVAL( riscv_readcsr ) },
  { LSTRKEY( "readmtime" ), LFUNCVAL( riscv_readmtime ) },
  { LSTRKEY( "readmcycle" ), LFUNCVAL( riscv_readmcycle ) },
 
#if LUA_OPTIMIZE_MEMORY > 0
 // { LSTRKEY( "__metatable" ), LROVAL( term_map ) },
  { LSTRKEY( "misa" ), LNUMVAL( CSR_MISA ) },
  { LSTRKEY( "mimpid" ), LNUMVAL( CSR_MIMPID ) },
  { LSTRKEY( "mstatus" ), LNUMVAL( CSR_MSTATUS ) },
  { LSTRKEY( "mie" ), LNUMVAL( CSR_MIE ) },
  { LSTRKEY( "mip" ), LNUMVAL( CSR_MIP ) },
 
#endif
 // { LSTRKEY( "__index" ), LFUNCVAL( term_mt_index ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_riscv(lua_State *L)
{
  LREGISTER(L,"riscv",riscv_map);	
}
