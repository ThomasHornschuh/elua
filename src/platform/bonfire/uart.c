
//    |--------------------|--------------------------------------------|
//--! | Address            | Description                                |
//--! |--------------------|--------------------------------------------|
//--! | 0x00               | Transmit register(W)/  Receive register(R) |
//--! | 0x04               | Status(R)) and control(W) register         |
//--! |--------------------|--------------------------------------------|

//--! The status register contains the following bits:
//--! - Bit 0: UART RX Ready bit. Reads as 1 when there's received data in FIFO, 0 otherwise.
//--! - Bit 1: UART TX Ready bit. Reads as 1 when there's space in TX FIFO for transmission, 0 otherwise.

// Control register:
// Bit [15:0] - UARTPRES UART prescaler (16 bits)   (f_clk / (baudrate * 16)) - 1
// Bit 16 - UARTEN UARTEN bit controls whether UART is enabled or not



#include <stdint.h>
#include <stdbool.h>


#include ELUA_CPU_HEADER
#include "platform.h" // eLua platform in this case !!!!
#include "uart.h"



#define UART_TX 0
#define UART_RECV 0
#define UART_STATUS 1
#define UART_CONTROL 1
//#define UART_INTE 0x10
//#define UART_REVISION 0x14

//#define ENABLE_SEND_DELAY 1


static volatile uint32_t *uartadr=(uint32_t *)UART_BASE;
//static volatile uint32_t *gpioadr=(uint32_t *)GPIO_BASE;



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
  while (!(uartadr[UART_STATUS] & 0x2)); // Wait while transmit buffer full
  uartadr[UART_TX]=(uint32_t)c;

}

char uart_readchar()
{
  while (!(uartadr[UART_STATUS] & 0x01)); // Wait while receive buffer empty
  return (char)uartadr[UART_RECV];
}



// adpated version for elua:
// if timeout==0 and data from the UART is available the data is returned, otherwise -1 is returned
// if timeout != 0 then it waits until data read from the UART becomes available
int uart_wait_receive(long timeout)
{
uint8_t status;

  do {
    status=uartadr[UART_STATUS];
    if  (status & 0x01) { // receive buffer not empty?
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


static uint16_t l_divisor=0;
void _setDivisor(uint32_t divisor){

   l_divisor = divisor;
   uartadr[UART_CONTROL]= 0x010000L | (uint16_t)divisor; // Set Baudrate divisor and enable port
}


void setDivisor(uint32_t divisor)
{
    _setDivisor(divisor);
    wait(1000000);
}

uint32_t getDivisor()
{
  return l_divisor;
}

void uart_setBaudRate(int baudrate) {
// sample_clk = (f_clk / (baudrate * 16)) - 1
// (96.000.000 / (115200*16))-1 = 51,08

   setDivisor(SYSCLK / (baudrate*16) -1);
}

uint8_t getUartRevision()
{
   return 0x0ff; // not supported with ZPUINO UART yet

}
