// RISC-V proxy-kernel syscall definitions for the Picolibc Spike build.

#ifndef ELUA_RISCV_SYSCALL_H
#define ELUA_RISCV_SYSCALL_H

#ifdef ELUA_PICOLIBC
#define SYS_exit          93
#define SYS_read          63
#define SYS_write         64
#define SYS_close         57
#define SYS_lseek         62
#define SYS_brk          214
#define SYS_gettimeofday 169

// riscv-pk still supports this legacy Newlib syscall number.
#define SYS_open        1024

static inline long __internal_syscall( long number, long arg0, long arg1,
                                       long arg2, long arg3 )
{
  register long a0 asm( "a0" ) = arg0;
  register long a1 asm( "a1" ) = arg1;
  register long a2 asm( "a2" ) = arg2;
  register long a3 asm( "a3" ) = arg3;
  register long a7 asm( "a7" ) = number;

  asm volatile( "ecall"
                : "+r"( a0 )
                : "r"( a1 ), "r"( a2 ), "r"( a3 ), "r"( a7 )
                : "memory" );
  return a0;
}
#else
#include_next <machine/syscall.h>
#endif

#endif
