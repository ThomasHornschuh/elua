// "Borrowed" from RISC-V proxy kernel
// See LICENSE for license details.

#include "platform_conf.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include <stdlib.h>
#include <ctype.h>
#include "uart.h"

#include "console.h"


static void vprintk(const char* s, va_list vl)
{
  char out[2048];

  vsnprintf(out, sizeof(out), s, vl);
  char *p=out;
  while (*p) {
    if (*p=='\n') platform_s_uart_send(0,'\r');
    platform_s_uart_send(0,*p);
    p++;
  }
}

void printk(const char* s, ...)
{
  va_list vl;
  va_start(vl, s);

  vprintk(s, vl);

  va_end(vl);
}

void dump_tf(trapframe_t* tf)
{
  static const char*  regnames[] = {
    "z ", "ra", "sp", "gp", "tp", "t0",  "t1",  "t2",
    "s0", "s1", "a0", "a1", "a2", "a3",  "a4",  "a5",
    "a6", "a7", "s2", "s3", "s4", "s5",  "s6",  "s7",
    "s8", "s9", "sa", "sb", "t3", "t4",  "t5",  "t6"
  };

  tf->gpr[0] = 0;

  for(int i = 0; i < 32; i+=4)
  {
    for(int j = 0; j < 4; j++)
      printk("%s %08lx%c",regnames[i+j],tf->gpr[i+j],j < 3 ? ' ' : '\n');
  }
  printk("pc %08lx va %08lx op %08lx sr %08lx\n\n\n", tf->epc, tf->badvaddr,
         (uint32_t)tf->insn, tf->status);
}

void do_panic(const char* s, ...)
{
  va_list vl;
  va_start(vl, s);

  vprintk(s, vl);
  asm("sbreak"); // Break into debugger

}

void kassert_fail(const char* s)
{
  register uintptr_t ra asm ("ra");
  do_panic("assertion failed @ %p: %s\n", ra, s);
}

