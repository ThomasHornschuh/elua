#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"
#include <reent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "console.h"
#include "common.h"

#include "uart.h"
#include "irq_handler.h"
#include "encoding.h"
#include <stdio.h>


volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE; // Pointer to memory mapped RISC-V Timer registers

uint32_t tick_interval=0;

#define __virt_timer_period ((long)(SYSCLK/VTMR_FREQ_HZ)) 


/*
 * Because currently only support for Lua interrupts on virtual timers is implemented,
 * the functions below will not be called. But eLua need the defintions to build
 * interrupt support. 
 * */


int int_tmr_match_set_status( elua_int_resnum resnum, int state)
{
	printk("int_tmr_match_set_status %d %d\n",resnum,state);
	return 0;
}


int int_tmr_match_get_status(elua_int_resnum resnum)
{
   printk("int_tmr_match_get_status %d %d\n",resnum);
   return 0;
}

int int_tmr_match_get_flag( elua_int_resnum resnum, int clear)
{
	 printk("int_tmr_match_get_flag %d %d\n",resnum,clear);
	 return 0;
}



const elua_int_descriptor elua_int_table[ INT_ELUA_LAST ] =
{
  //{ int_gpio_posedge_set_status, int_gpio_posedge_get_status, int_gpio_posedge_get_flag },
  //{ int_gpio_negedge_set_status, int_gpio_negedge_get_status, int_gpio_negedge_get_flag },
  { int_tmr_match_set_status, int_tmr_match_get_status, int_tmr_match_get_flag }
};


/* Use the RISC-V standard timer as base timer for the virtual timers */

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

void timer_irq_handler()
{	
   pmtime[2]=pmtime[0]+tick_interval;  // Will as side effect clear the pending irq
   cmn_virtual_timer_cb(); // call eLua
} 



void platform_int_init()
{
   printk("__virt_timer_period %ld\n",__virt_timer_period);
   mtime_setinterval(__virt_timer_period);
   set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable
}


// ****************************************************************************
// CPU functions


int platform_cpu_set_global_interrupts( int status )
{
  
  if (status==PLATFORM_CPU_ENABLE)
    set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable	
  else
    clear_csr(mstatus,MSTATUS_MIE); // Global Interrupt Diable	
    
  return PLATFORM_OK;
}

int platform_cpu_get_global_interrupts( void )
{
uint32_t m = read_csr(mstatus);
   
   if (m & MSTATUS_MIE)
     return 1;
   else  	
    return 0;
}


