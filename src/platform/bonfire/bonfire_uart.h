#ifndef __BONFIRE_UART_H__
#define __BONFIRE_UART_H__


#define UART_TXRX 0
#define UART_STATUS 1
#define UART_EXT_CONTROL 2
#define UART_INT_REGISTER 3

#define BIT_FIFO_INT_PENDING 19
#define BIT_FIFO_INT_ENABLE 3

uint32_t* get_uart_base(unsigned id);

#endif
