// Platform-dependent functions

#include "platform_conf.h"

#include "genstd.h"
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

#include "gdb_interface.h"


// ****************************************************************************
// Platform initialization (low-level and full)

//void *memory_start_address = 0;
//void *memory_end_address = 0;

extern char end; // From linker script, end of data segment



void platform_ll_init( void )
{
 
  // Uncomment to break into debugger on boot 
  // gdb_setup_interface(GDB_DEBUG_UART,GDB_DEBUG_BAUD);
  // printk("Connect debugger with %d baud\n",GDB_DEBUG_BAUD );
  // gdb_enable_debugger();
  // gdb_breakpoint();
  // gdb_breakpoint();
}

// #ifdef ULX3S
// static void init_sd()
// {
// u8 r;

//     u32 i=platform_spi_setup( 1, PLATFORM_SPI_MASTER, 400000, 0, 0, 8 );
//     printk("MMC SPI init clock set to %ld Hz\n",i);
//     for(int k=1;k<10;k++) {     
//       //printk("Send inital clock train\n");
//       platform_spi_select(1,PLATFORM_SPI_DISABLE);    
//       for (i=1;i<=10;i++) {
//         platform_spi_send_recv(1,0xff);
//       }
//       //printk("Send CMD0\n");
//       platform_spi_select(1,PLATFORM_SPI_ENABLE);
//       platform_spi_send_recv(1,0x40);
//       platform_spi_send_recv(1,0);
//       platform_spi_send_recv(1,0);
//       platform_spi_send_recv(1,0);
//       platform_spi_send_recv(1,0); 
//       platform_spi_send_recv(1,0x95);
      
//       i=10;
//       do {
//         r=platform_spi_send_recv(1,0xff);
//       } while ((r & 0x80) && --i);
//       platform_spi_select(1,PLATFORM_SPI_DISABLE);
      
//       if (r==1) break;
//     }
//     printk("SD Card return %0x\n",r);
// }

// #endif


int platform_init()
{
 
  cmn_systimer_set_base_freq(SYSCLK);

#ifdef  BUILD_UIP
  platform_eth_init();
#endif

   #if NUM_PIO>0
    // GPIO Input is always enabled
    _write_word((void*)GPIO_BASE+GPIO_INPUT_EN,0xffffffff);
    #ifdef ULX3S
       // Set SD(2) pin high, it has to be low on ESP32 boot, but high for SD card usage
       // Set Wifi EN to 1 to keep ESP32 running
       // First set output val, then enable output to avoid glitch on Wifi_EN
       _set_bit((void*)GPIO_BASE+GPIO_OUTPUT_VAL,11);
       _set_bit((void*)GPIO_BASE+GPIO_OUTPUT_EN,11);
       //init_sd();
       
    #endif
   #endif

  cmn_platform_init(); // call the common initialiation code
  printk("eLua for Bonfire SoC \n");
  printk("Build with GCC %s\n",__VERSION__);
  printk("Heap: %08lx .. %08lx\n",&INTERNAL_RAM1_FIRST_FREE,INTERNAL_RAM1_LAST_FREE);
  return PLATFORM_OK;
}



// ****************************************************************************
// UART functions


#ifdef ZPUINO_UART

#if defined(UART_BASE_ADDR) 
#pragma message "Dynamic UART config active"

static uint32_t uart_bases[] = UART_BASE_ADDR;

uint32_t* get_uart_base(unsigned id) {
   
   if (id>NUM_UART-1) 
     return 0;
   else
     return (uint32_t*)uart_bases[id];  
}  

#else

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

#endif // ZPUINO_UART

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
  return baud;
#else
volatile uint32_t *uartadr=get_uart_base(id);

  if (uartadr) {
     uint32_t ft;
     #ifdef UART_FIFO_THRESHOLD
       ft= UART_FIFO_THRESHOLD<<18;
     #else
       ft=0;
     #endif

     //avoid chaning Baudrate while transmit in progress transmitter ready
       while (!(uartadr[UART_STATUS] & 0x2)); 

     // Set Baudrate divisor, enable port, extended mode and set FIFO Threshold
     #ifdef OLD_ZPUINO_UART
       uartadr[UART_EXT_CONTROL]= 0x030000L | ft | (uint16_t)(SYSCLK / (baud*16) -1);
     #else 
       uartadr[UART_EXT_CONTROL]= 0x030000L | ft | (uint16_t)(SYSCLK / baud -1);
     #endif 
     return baud;
  } else 
    return 0;
#endif
  
}

void platform_s_uart_send( unsigned id, u8 data )
{
#ifndef ZPUINO_UART

   uart_writechar((char)data);
#else

volatile uint32_t *uartadr=get_uart_base(id);

  if (uartadr) {
    while (!(uartadr[UART_STATUS] & 0x2)); // Wait until transmitter ready
    uartadr[UART_TXRX]=(uint32_t)(data & 0x0ff);
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
