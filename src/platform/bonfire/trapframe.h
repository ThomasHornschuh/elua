#ifndef __TRAPFRAME_H
#define __TRAPFRAME_H

#include <stdint.h>

#ifdef __riscv64
  #define TREG uint64_t
#else
  #define TREG uint32_t
#endif

typedef struct
{
  TREG gpr[32];
  TREG status;
  TREG epc;
  TREG badvaddr;
  TREG cause;
  TREG insn;
} trapframe_t;



typedef  trapframe_t* (*t_ptrapfuntion)(trapframe_t*);

#endif
