// Platform-dependent functions

#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"
#include <reent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "term.h"
#include "console.h"
#include "common.h"

#include "uart.h"
#include "irq_handler.h"
#include "encoding.h"

#include <stdio.h>



trapframe_t* trap_handler(trapframe_t *ptf)
{
char c;
   
    if (ptf->cause & 0x80000000) {
       // interrupt   
       if ((ptf->cause & 0x0ff)==7)
         timer_irq_handler();
       else 
         printk("Unexepted Interrupt cause %x\n",ptf->cause);    
        
      // interrupts must not increment epc !!!
        
    }  else {  
     // trap   
  
		printk("\nTrap cause: %lx\n",ptf->cause);
		dump_tf(ptf);
		c=uart_readchar();
		if (c=='r' || c=='R')
		  ptf->epc=SRAM_BASE; // will cause reset
		else
		  ptf->epc+=4;
        
    }    
    return ptf;
}


// ****************************************************************************
// Platform initialization (low-level and full)

//void *memory_start_address = 0;
//void *memory_end_address = 0;

extern char end; // From linker script, end of data segment




void platform_ll_init( void )
{
  printk("Heap: %08lx .. %08lx\n",&INTERNAL_RAM1_FIRST_FREE,INTERNAL_RAM1_LAST_FREE);
  
	   
}

int platform_init()
{ 
#define __sys_timer_period ((long)((double)(4294967296.0 / SYSCLK *1000000.0)))  // for 96Mhz Clock it should be 44739242

 
  uart_write_console("eLua for Bonfire SoC 1.0\n");	
  printk("Seting System Timer Period to %ld\n",__sys_timer_period);
  cmn_systimer_set_base_freq(SYSCLK);
  cmn_systimer_set_interrupt_period_us(__sys_timer_period);
  
  set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable
  platform_timer_sys_enable_int();
 

  cmn_platform_init(); // call the common initialiation code
  return PLATFORM_OK;
}

// ****************************************************************************
// "Dummy" UART functions

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{
  //printk("UART Setup %d %d %d %d %d\n",id,baud,databits,parity,stopbits);	
  uart_setBaudRate(baud);
  return baud;
}

void platform_s_uart_send( unsigned id, u8 data )
{
	uart_writechar(data);
}

int platform_s_uart_recv( unsigned id, timer_data_type timeout )
{
  return uart_wait_receive(timeout);
}

int platform_s_uart_set_flow_control( unsigned id, int type )
{
  return PLATFORM_ERR;
}

// ****************************************************************************
// "Dummy" timer functions

void platform_s_timer_delay( unsigned id, timer_data_type delay_us )
{
}

timer_data_type platform_s_timer_op( unsigned id, int op, timer_data_type data )
{
  return 0;
}

timer_data_type platform_timer_read_sys( void )
{
  return cmn_systimer_get();
}

// ****************************************************************************
// "Dummy" CPU functions


int platform_cpu_set_global_interrupts( int status )
{
  
  if (status==PLATFORM_CPU_ENABLE)
    set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable	
  else
    clear_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable	
    
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

