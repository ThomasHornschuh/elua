#include "platform.h"
#include "platform_conf.h"
#include "common.h"

#include "type.h"
#include "devman.h"
#include "genstd.h"
#include "encoding.h"
#include "uart.h"


volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE;

uint32_t tick_interval=0;

u64 platform_timer_sys_raw_read()
{ 
#if __riscv_xlen == 32
  while (1) {
    uint32_t hi = read_csr(mcycleh);
    uint32_t lo = read_csr(mcycle);
    if (hi == read_csr(mcycleh))
      return ((uint64_t)hi << 32) | lo;
  }
#else
  return read_csr(mcycle);
#endif

}


uint32_t mtime_setinterval(uint32_t interval)
{
// Implementation for 32 Bit timer in Bonfire. Need to be adapted in case of a 64Bit Timer

   tick_interval=interval;	
   
   
   if (interval >0) {
     pmtime[2]=pmtime[0]+interval; 
     set_csr(mie,MIP_MTIP); // Enable Timer Interrupt	
     
   } else {
	 clear_csr(mie,MIP_MTIP); // Disable Timer Interrupt	
	 
   }
   return tick_interval;	   	   	
}

void platform_timer_sys_enable_int()
{
  // empty because we never need an overflow interrupt
   	
  //uart_write_console("platform_timer_sys_enable_int\n");		
  //pmtime[2]=0x0ffffffff; // Set Timer compare value	
  //set_csr(mie,MIP_MTIP); // Enable Timer Interrupt	
}



void platform_timer_sys_disable_int()
{
   // empty because we never need an overflow interrupt
	
   //uart_write_console("platform_timer_sys_disable_int\n");	
   //clear_csr(mie,MIP_MTIP); // Enable Timer Interrupt	
}

void timer_irq_handler()
{	
   pmtime[2]=pmtime[0]+tick_interval;  // Will as side effect clear the pending irq
   cmn_virtual_timer_cb(); // call eLua
}
