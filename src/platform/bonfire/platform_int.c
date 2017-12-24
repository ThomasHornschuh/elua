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


#include "bonfire_uart.h"

#include "uart.h" // depreciated !!!!

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
static void ext_irq_handler()
{
uint32_t pending;

   pending=read_csr(mip);
   if (pending & MIP_MEIP) {
     ethernet_irq_handler();
   } else
     printk("Unexepted external Interrupt MIP= %x\n",pending);

}

#endif

#ifdef INT_UART_RX_FIFO

static int uart_irq_table[1]= {UART0_INTNUM}; // LIRQ "Offsets" for UARTs

static int pending[NUM_UART];
static int enabled[NUM_UART];


#define FIFO_INT_MASK (1<<BIT_FIFO_INT_PENDING)

static void  uart_irq_handler(int cause)
{
int i;
volatile uint32_t *uart_base;

   for(i=0;i<NUM_UART;i++) {
      if (cause==uart_irq_table[i]) {
         uart_base=get_uart_base(i);
         if (!uart_base) kassert_fail("Unexpected UART interrupt");
         // read FIFO nearly full interrupt and disable all further interrupts
         if  (uart_base[UART_INT_REGISTER] & FIFO_INT_MASK) {
           pending[i]=1;
           uart_base[UART_INT_REGISTER]= FIFO_INT_MASK; // Clear and disable
           cmn_int_handler(INT_UART_RX_FIFO,i);
         }
      }
   }
}

int int_uart_rx_fifo_set_status( elua_int_resnum resnum, int state)
{
int old = enabled[resnum];
volatile uint32_t *uart_base;

    enabled[resnum]=state;
    uart_base=get_uart_base(resnum);
    if (uart_base) {
       if (state) {
         uart_base[UART_INT_REGISTER]=1<<BIT_FIFO_INT_ENABLE;
         set_csr(mie,1<<uart_irq_table[resnum]);
       } else {
         uart_base[UART_INT_REGISTER]=0;
         clear_csr(mie,1<<uart_irq_table[resnum]);
       }
    }

    return old;
}


int int_uart_rx_fifo_get_status(elua_int_resnum resnum)
{
   return enabled[resnum];
}

int int_uart_rx_fifo_get_flag( elua_int_resnum resnum, int clear)
{
int res = pending[resnum];
volatile uint32_t *uart_base;

     if (clear) {
       pending[resnum]=0;
       // clearing the "soft" pending flag triggers also reactivating
       // the hardware interrupt if enabled
       if (enabled[resnum]) {
          uart_base=get_uart_base(resnum);
          if (uart_base) uart_base[UART_INT_REGISTER]=1<<BIT_FIFO_INT_ENABLE;
       }
     }
     return res;
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
#ifdef INT_UART_RX_FIFO
  ,
  { int_uart_rx_fifo_set_status, int_uart_rx_fifo_get_status,int_uart_rx_fifo_get_flag }
#endif
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





void platform_int_init()
{

   printk("__virt_timer_period %ld\n",__virt_timer_period);
   mtime_setinterval(__virt_timer_period);

#ifdef INT_UART_RX_FIFO
   int i;
   for(i=0;i<NUM_UART;i++) {
      int_uart_rx_fifo_set_status(i,0);
      int_uart_rx_fifo_get_flag(i,1);
   }
#endif
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

#ifdef INT_UART_RX_FIFO
         case 16+6: // Local Interrupt 6 (lxp irq_i(7))
         case 16+5: // Local Interrupt 5 (lxp irq_i(6))
           uart_irq_handler(ptf->cause & 0x0ff);
           break;
#endif

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
        c=platform_s_uart_recv(0,1);
        if (c=='r' || c=='R')
          ptf->epc=SRAM_BASE; // will cause reset
        else
          ptf->epc+=4;

    }
    return ptf;
}

