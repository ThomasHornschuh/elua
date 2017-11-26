#ifndef __IRQ_HANDLER_H
#define __IRQ_HANDLER_H


#include "trapframe.h"

void ethernet_irq_handler();

extern volatile uint32_t eth_timer_fired;



#endif
