/* Test UART */
// sample_clk = (f_clk / (baudrate * 16)) - 1
// (32.000.000 / (115200*16))-1 = 16,36 ...
// UART Base: 0x20000000
//    |--------------------|--------------------------------------------|
//--! | Address            | Description                                |
//--! |--------------------|--------------------------------------------|
//--! | 0x00               | Transmit register (write-only)             |
//--! | 0x04               | Receive register (read-only)               |
//--! | 0x08               | Status register (read-only)                |
//--! | 0x0c               | Sample clock divisor register (read/write) |
//--! | 0x10               | Interrupt enable register (read/write)     |
// !  | 0x14               | Revision Code                              |
//--! |--------------------|--------------------------------------------|

//--! The status register contains the following bits:
//--! - Bit 0: receive buffer empty
//--! - Bit 1: transmit buffer empty
//--! - Bit 2: receive buffer full
//--! - Bit 3: transmit buffer full

#include <stdint.h>
#include <stdbool.h>


#include ELUA_CPU_HEADER
#include "platform.h" // eLua platform in this case !!!!
#include "uart.h"

#define UART_TX 0x0
#define UART_RECV 0x04
#define UART_STATUS 0x08
#define UART_DIVISOR 0x0c
#define UART_INTE 0x10
#define UART_REVISION 0x14

#define ENABLE_SEND_DELAY 0


volatile uint8_t *uartadr=(uint8_t *)UART_BASE;

volatile uint8_t *gpioadr=(uint8_t *)GPIO_BASE;


void wait(long nWait)
{
static volatile int c;

  c=0;
  while (c++ < nWait);
}



void uart_writechar(char c)
{

#ifdef  ENABLE_SEND_DELAY
   wait(1000);
#endif
  while (uartadr[UART_STATUS] & 0x08); // Wait while transmit buffer full
  uartadr[UART_TX]=(uint8_t)c;

}



char uart_readchar()
{
  while (uartadr[UART_STATUS] & 0x01); // Wait while receive buffer empty
  return uartadr[UART_RECV];
}


// adpated version for elua:
// if timeout==0 and data from the UART is available the data is returned, otherwise -1 is returned
// if timeout != 0 then it waits until data read from the UART becomes available
int uart_wait_receive(long timeout)
{
uint8_t status;

  do {
    status=uartadr[UART_STATUS];
    if ((status & 0x01)==0) { // receive buffer not empty?
      return uartadr[UART_RECV];
    }

  }while(timeout);
  return -1;
}



void uart_write_str(char *p)
{
  while (*p) {
    uart_writechar(*p);
    p++;
  }
}


// Like Writestr but expands \n to \n\r
void uart_write_console(char *p)
{
   while (*p) {
    if (*p=='\n') uart_writechar('\r');
    uart_writechar(*p);
    p++;
  }

}



void _setDivisor(uint32_t divisor){

   uartadr[UART_DIVISOR]=divisor; // Set Baudrate divisor
}

void setDivisor(uint32_t divisor)
{
    _setDivisor(divisor);
    wait(1000000);
}

uint32_t getDivisor()
{
  return uartadr[UART_DIVISOR];
}

void uart_setBaudRate(int baudrate) {
// sample_clk = (f_clk / (baudrate * 16)) - 1
// (96.000.000 / (115200*16))-1 = 51,08

   setDivisor(SYSCLK / (baudrate*16) -1);
}

uint8_t getUartRevision()
{
   return uartadr[UART_REVISION];

}
