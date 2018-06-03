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
#include "bonfire_gpio.h"
#include "mem_rw.h"


#include "irq_handler.h"
#include "encoding.h"

#include <stdio.h>

#ifdef BUILD_UIP
#include "xil_etherlite.h"
#endif

#ifndef ZPUINO_UART
#include "uart.h"
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

  printk("eLua for Bonfire SoC 1.0a\n");
  printk("Build with GCC %s\n",__VERSION__);
  cmn_systimer_set_base_freq(SYSCLK);

#ifdef BUILD_UIP
  platform_eth_init();
#endif

   #if NUM_PIO>0
    // GPIO Input is always enabled
    _write_word((void*)GPIO_BASE+GPIO_INPUT_EN,0xffffffff);
   #endif

  cmn_platform_init(); // call the common initialiation code
  return PLATFORM_OK;
}



// ****************************************************************************
// UART functions



#ifdef ZPUINO_UART
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
#endif

// Extended Control register:
// Bit [15:0] - UARTPRES UART prescaler (16 bits)   (f_clk / baudrate ) - 1
// Bit 16 - UARTEN UARTEN bit controls whether UART is enabled or not
// Bit 17 - EXT_EN: Enable extended mode - when set one the extended mode is activated
// Bit [31..18] - FIFO "Nearly Full" Threshold. The number of bits actual used depends on the confirued FIFO size

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{

  //printk("Platform UART setup id: %d, baud :%d",id,baud);
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
     uartadr[UART_EXT_CONTROL]= 0x030000L | ft | (uint16_t)(SYSCLK / baud -1);

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

// ****************************************************************************
// PIO Functions

#if NUM_PIO>0

#pragma message "compiling PIO functions"

static int port_shift[] = PORT_SHIFT; // Bit offsets of the ports in the GPIO module

// return a mask with bits for a given port set to 1
static pio_type gen_port_mask(unsigned port)
{
static int pins[] = PIO_PIN_ARRAY;
int i;
pio_type mask=0;
int left=port_shift[port];

  for(i=left;i<pins[port]+left;i++)
    mask|= 1 << i;

  return mask;
}


pio_type platform_pio_op( unsigned port, pio_type pinmask, int op )
{

pio_type shifted_pinmask = pinmask << port_shift[port];

pio_type temp,pmask;

  switch(op) {
     case PLATFORM_IO_PIN_SET:
       _mem_or((void*)GPIO_BASE+GPIO_OUTPUT_VAL,shifted_pinmask); break;
     case PLATFORM_IO_PIN_CLEAR:
       _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_VAL,~shifted_pinmask); break;
     case PLATFORM_IO_PIN_GET:
       return _read_word((void*)GPIO_BASE+GPIO_INPUT_VAL) & shifted_pinmask?1:0;
     case PLATFORM_IO_PIN_DIR_INPUT:
        _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_EN,~shifted_pinmask);
        //_mem_or((void*)GPIO_BASE+GPIO_INPUT_EN,shifted_pinmask);
        break;
     case PLATFORM_IO_PIN_DIR_OUTPUT:
       //_mem_and((void*)GPIO_BASE+GPIO_INPUT_EN,~shifted_pinmask);
       _mem_or((void*)GPIO_BASE+GPIO_OUTPUT_EN,shifted_pinmask);
       break;
     case  PLATFORM_IO_PORT_SET_VALUE:
       pmask=gen_port_mask(port);
       temp=_read_word((void*)GPIO_BASE+GPIO_OUTPUT_VAL);
       temp = (temp & ~pmask) | shifted_pinmask;
       _write_word((void*)GPIO_BASE+GPIO_OUTPUT_VAL,temp);
       break;
     case  PLATFORM_IO_PORT_GET_VALUE:
       return (_read_word((void*)GPIO_BASE+GPIO_INPUT_VAL) & gen_port_mask(port)) >> port_shift[port];
     case PLATFORM_IO_PORT_DIR_INPUT:
        temp=gen_port_mask(port);
        _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_EN,~temp);
        //_mem_or((void*)GPIO_BASE+GPIO_INPUT_EN,temp);
        break;
     case PLATFORM_IO_PORT_DIR_OUTPUT:
        temp=gen_port_mask(port);
        //_mem_and((void*)GPIO_BASE+GPIO_INPUT_EN,~temp);
        _mem_or((void*)GPIO_BASE+GPIO_OUTPUT_EN,temp);
        break;
     default:
       return 0;
  }
  return 1;
}

#endif
