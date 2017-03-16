#include "platform.h"
#include "platform_conf.h"
#include "common.h"

#include "type.h"
#include "devman.h"
#include "genstd.h"
#include "encoding.h"


volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE;

u64 platform_timer_sys_raw_read()
{
  return pmtime[0];	
}



void platform_timer_sys_enable_int()
{
  pmtime[2]=0x0ffffffff; // Set Timer compare value	
  set_csr(mie,MIP_MTIP); // Enable Timer Interrupt	
}



void platform_timer_sys_disable_int()
{
   clear_csr(mie,MIP_MTIP); // Enable Timer Interrupt	
}

void timer_irq_handler()
{
	uart_write_console("timer irq\n");	
  	pmtime[2]=0x0ffffffff; // Will as side effect clear the pedning irq
  	cmn_systimer_periodic(); // call eLua
	
}
