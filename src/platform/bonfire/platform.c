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

#ifdef BUILD_UIP
#include "xil_etherlite.h"
#endif



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

  uart_write_console("eLua for Bonfire SoC 1.0a\n");
  printk("Build with GCC %s\n",__VERSION__);
  cmn_systimer_set_base_freq(SYSCLK);

#ifdef BUILD_UIP
  platform_eth_init();
#endif

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

int platform_s_timer_set_match_int( unsigned id, timer_data_type period_us, int type )
{
  //printk("platform_s_timer_set_match_int called, should not happen...");
  return 0;
}


// ****************************************************************************
// System Timer functions

timer_data_type platform_timer_read_sys( void )
{
  return cmn_systimer_get();
}


