#ifndef _STACKS_H
#define _STACKS_H

#define  STACK_SIZE_USR   65536
#define  STACK_SIZE_IRQ   2048
#define  STACK_SIZE_TOTAL ( STACK_SIZE_USR + STACK_SIZE_IRQ )

#define STACK_TOP (DRAM_TOP & 0x0fffffff0) 

#endif
