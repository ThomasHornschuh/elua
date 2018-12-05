#include ELUA_CPU_HEADER


#include <stdio.h>

#include "platform.h"

#include "console.h"


#include "riscv-gdb-stub.h"


static  uint32_t uart_id=GDB_DEBUG_UART;


int getDebugChar() {

  return platform_s_uart_recv(uart_id,PLATFORM_TIMER_INF_TIMEOUT);

};

void putDebugChar(int c) {

  platform_s_uart_send(uart_id,c);
};



u32 gdb_setup_interface(u32 port, u32 baudrate) {

   uart_id=port;
   platform_uart_set_buffer(port,0); // disable buffer 
   return platform_uart_setup(port,baudrate,8,PLATFORM_UART_PARITY_NONE,PLATFORM_UART_STOPBITS_1);
}





