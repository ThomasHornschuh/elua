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

#ifdef BUILD_IP
#include "elua_uip.h"
#endif

#include "riscv-gdb-stub.h"
t_ptrapfuntion gdb_debug_handler=NULL;

volatile uint32_t *pmtime = (uint32_t*)MTIME_BASE; // Pointer to memory mapped RISC-V Timer registers

uint32_t tick_interval=0;

volatile uint32_t eth_timer_fired=0;

#define __virt_timer_period ((long)(SYSCLK/VTMR_FREQ_HZ))


#ifdef __ARTY_H
void ext_irq_handler()
{
uint32_t pending;

   pending=read_csr(mip);
   if (pending & MIP_MEIP) {
     ethernet_irq_handler();
   } else
     printk("Unexepted external Interrupt MIP= %x\n",pending);

}

#endif

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

#ifdef  BUILD_UIP
   // Indicate that a SysTick interrupt has occurred.
    eth_timer_fired = 1;

    // Call the net mainloop.  This will perform the actual work
    // of incrementing the timers and taking the appropriate actions.
    // Because we are already at irq level we can avoid the overhead of
    // calling the loop indirectly with platform_eth_force_interrupt

    elua_uip_mainloop();
#endif

   cmn_virtual_timer_cb(); // call eLua
}



//#define DEBUG_BAUD 115200

//void init_gdb_stub()
//{
  //gdb_setup_interface(DEBUG_BAUD);
  //gdb_debug_handler=gdb_initDebugger(0);
  //printk("Connect with Debugger port with %d baud\n",DEBUG_BAUD);
  //gdb_breakpoint();

//}

void platform_int_init()
{
   //init_gdb_stub();
   printk("__virt_timer_period %ld\n",__virt_timer_period);
   mtime_setinterval(__virt_timer_period);

   set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable
}


// ****************************************************************************
// CPU functions


int platform_cpu_set_global_interrupts( int status )
{
int result = platform_cpu_get_global_interrupts();

  if (status==PLATFORM_CPU_ENABLE)
    set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable
  else
    clear_csr(mstatus,MSTATUS_MIE); // Global Interrupt Diable

  return result;
}

int platform_cpu_get_global_interrupts( void )
{
uint32_t m = read_csr(mstatus);

   if (m & MSTATUS_MIE)
     return PLATFORM_CPU_ENABLE;
   else
    return PLATFORM_CPU_DISABLE;
}


trapframe_t* trap_handler(trapframe_t *ptf)
{
char c;

    if (ptf->cause & 0x80000000) {
       // interrupt
       switch (ptf->cause & 0x0ff) {
         case 0x07:
           timer_irq_handler();
           break;

#ifdef __ARTY_H
         case 0x0b:
           ext_irq_handler();
           break;
#endif
         default:
           printk("Unexepted Interrupt cause %x\n",ptf->cause);
      }

      // interrupts must not increment epc !!!

    }  else {
     // trap

        if (gdb_debug_handler) return gdb_debug_handler(ptf);

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

