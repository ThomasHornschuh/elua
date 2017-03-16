#ifndef __UART_H

#define __UART_H

#include <stdint.h>

void uart_setBaudRate(int baudrate);
void uart_writechar(char c);
char uart_readchar();
int uart_wait_receive(long timeout);

void uart_write_str(char *p);
void uart_write_console(char *p);

//void writeHex(uint32_t v);

//void setDivisor(uint32_t divisor);
//void _setDivisor(uint32_t divisor);
//uint32_t getDivisor();


//void wait(long nWait);
uint8_t getUartRevision();


#endif
