#ifndef __IRQ_HANDLER_H
#define __IRQ_HANDLER_H


typedef struct
{
  long gpr[32];
  long status;
  long epc;
  long badvaddr;
  long cause;
  long insn;
} trapframe_t;


void timer_irq_handler();

#endif
