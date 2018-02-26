#include ELUA_CPU_HEADER


#include <stdio.h>

#include "console.h"

#define UART_TX 0
#define UART_RECV 0
#define UART_STATUS 1
#define UART_CONTROL 1




static volatile uint32_t *uartadr=(uint32_t *)UART1_BASE;


int getDebugChar() {

 while (!(uartadr[UART_STATUS] & 0x01)); // Wait while receive buffer empty
 return uartadr[UART_RECV] & 0x0ff;

};

void putDebugChar(int c) {
  while (!(uartadr[UART_STATUS] & 0x2)); // Wait while transmit buffer full
  uartadr[UART_TX]=(uint32_t)c;
};


static void setDivisor(uint32_t divisor){


   uartadr[UART_CONTROL]= 0x010000L | (uint16_t)divisor; // Set Baudrate divisor and enable port
}

void gdb_setup_interface(int baudrate) {


   setDivisor(SYSCLK / (baudrate*16) -1);
}





