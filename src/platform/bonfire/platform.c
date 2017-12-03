// Platform-dependent functions

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


#include "bonfire_uart.h"


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

extern  int initgdbserver( lua_State* L );

void platform_ll_init( void )
{
  printk("Heap: %08lx .. %08lx\n",&INTERNAL_RAM1_FIRST_FREE,INTERNAL_RAM1_LAST_FREE);
  //initgdbserver(NULL);

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
// UART functions




uint32_t* get_uart_base(unsigned id) {

  if (id>NUM_UART-1) return 0;
  switch(id) {
    case 0:
      return (uint32_t*)UART0_BASE;
    case 1:
      return (uint32_t*)UART1_BASE;
    default:
     return 0;
  }
}


// Extended Control register:
// Bit [15:0] - UARTPRES UART prescaler (16 bits)   (f_clk / (baudrate * 16)) - 1
// Bit 16 - UARTEN UARTEN bit controls whether UART is enabled or not
// Bit 17 - EXT_EN: Enable extended mode - when set one the extended mode is activated
// Bit [31..18] - FIFO "Nearly Full" Threshold. The number of bits actual used depends on the confirued FIFO size

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{

#ifndef ZPUINO_UART

  uart_setBaudRate(baud);
#else
volatile uint32_t *uartadr=get_uart_base(id);

  if (uartadr) {
     uint32_t ft;
     #ifdef UART_FIFO_THRESHOLD
       ft= UART_FIFO_THRESHOLD<<18;
     #else
       ft=0;
     #endif
     // Set Baudrate divisor, enable port, extended mode and set FIFO Threshold
     uartadr[UART_EXT_CONTROL]= 0x030000L | ft | (uint16_t)(SYSCLK / (baud*16) -1);

  }
#endif
  return baud;
}

void platform_s_uart_send( unsigned id, u8 data )
{
#ifndef ZPUINO_UART

   uart_writechar((char)data);
#else

volatile uint32_t *uartadr=get_uart_base(id);

  if (uartadr) {
    while (!(uartadr[UART_STATUS] & 0x2)); // Wait until transmitter ready
    uartadr[UART_TXRX]=(uint32_t)data;
  }
#endif
}



int platform_s_uart_recv( unsigned id, timer_data_type timeout )
{

#ifndef ZPUINO_UART
  return uart_wait_receive(timeout);
#else
volatile uint32_t *uartadr=get_uart_base(id);

  if (uartadr) {
   uint32_t status;
   do {
     status=uartadr[UART_STATUS];
     if  (status & 0x01) { // receive buffer not empty?
       return uartadr[UART_TXRX];
     }
   }while(timeout);
  }
  return -1;
#endif
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


