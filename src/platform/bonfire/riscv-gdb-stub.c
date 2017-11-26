/****************************************************************************

        THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <string.h>
#include <signal.h>
#include <stdint.h>

#include "bits.h"
#include "encoding.h"

#include "trapframe.h"

#include ELUA_CPU_HEADER

#include "platform.h"

/************************************************************************
 *
 * external low-level support routines
 */

//#define DEBUG 1

#include "console.h"
#ifdef DEBUG
#include <stdio.h>

#define PRINTK(...) printk(__VA_ARGS__)

#else
#define PRINTK(...)
#endif


extern void putDebugChar(); /* write a single character      */
extern int getDebugChar();  /* read and return a single char */

extern void exceptionHandler(int exception_number,void *execption_address);

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 2048


static const char hexchars[]="0123456789abcdef";

#define NUMREGS 32

enum {
  RISCV_ZERO_REGNUM = 0,    /* Read-only register, always 0.  */
  RISCV_RA_REGNUM = 1,      /* Return Address.  */
  RISCV_SP_REGNUM = 2,      /* Stack Pointer.  */
  RISCV_GP_REGNUM = 3,      /* Global Pointer.  */
  RISCV_TP_REGNUM = 4,      /* Thread Pointer.  */
  RISCV_FP_REGNUM = 8,      /* Frame Pointer.  */
  RISCV_A0_REGNUM = 10,     /* First argument.  */
  RISCV_A1_REGNUM = 11,     /* Second argument.  */
  RISCV_PC_REGNUM = 32,     /* Program Counter.  */

  RISCV_FIRST_FP_REGNUM = 33,   /* First Floating Point Register */
  RISCV_FA0_REGNUM = 49,
  RISCV_FA1_REGNUM = 50,
  RISCV_LAST_FP_REGNUM = 64,    /* Last Floating Point Register */

  RISCV_FIRST_CSR_REGNUM = 65,  /* First CSR */

  RISCV_LAST_CSR_REGNUM = 4160,

  RISCV_PRIV_REGNUM = 4161,

  /* Leave this as the last enum.  */
  RISCV_NUM_REGS
};




uint32_t flush_cache()
{
    //PRINTK("Flushing DCACHE...");
#if defined(DCACHE_SIZE)
#pragma message "implementing DCache Flush"
uint32_t *pmem = (void*)(DRAM_TOP-DCACHE_SIZE+1);

static volatile uint32_t sum=0; // To avoid optimizing away code below

  while ((uint32_t)pmem < DRAM_TOP) {
    sum+= *pmem++;
  }
#endif
  //PRINTK("OK\n");

  return 0;
}



/* Convert ch from a hex digit to an int */

static int
hex (unsigned char ch)
{
  if (ch >= 'a' && ch <= 'f')
    return ch-'a'+10;
  if (ch >= '0' && ch <= '9')
    return ch-'0';
  if (ch >= 'A' && ch <= 'F')
    return ch-'A'+10;
  return -1;
}

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum>     */

static char * getpacket (void)
{
  char *buffer = &remcomInBuffer[0];
  unsigned char checksum;
  unsigned char xmitcsum;
  int count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugChar ()) != '$')
    ;

retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX - 1)
    {
      ch = getDebugChar ();
          if (ch == '$')
            goto retry;
      if (ch == '#')
        break;
      checksum = checksum + ch;
      buffer[count] = ch;
      count = count + 1;
    }
      buffer[count] = 0;

      if (ch == '#')
    {
      ch = getDebugChar ();
      xmitcsum = hex (ch) << 4;
      ch = getDebugChar ();
      xmitcsum += hex (ch);

      if (checksum != xmitcsum)
        {
          putDebugChar ('-');   /* failed checksum */
        }
      else
        {
          putDebugChar ('+');   /* successful transfer */

          /* if a sequence char is present, reply the sequence ID */
          if (buffer[2] == ':')
        {
          putDebugChar (buffer[0]);
          putDebugChar (buffer[1]);

          return &buffer[3];
        }
          return &buffer[0];
        }
    }
    }
}

/* send the packet in buffer.  */

static void putpacket ( char *buffer)
{
  unsigned char checksum;
  int count;
  unsigned char ch;

  /*  $<packet info>#<checksum>. */
  PRINTK("> %s\n",buffer);
  do
    {
      putDebugChar('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
    {
      putDebugChar(ch);
      checksum += ch;
      count += 1;
    }

      putDebugChar('#');
      putDebugChar(hexchars[checksum >> 4]);
      putDebugChar(hexchars[checksum & 0xf]);

    }
  while (getDebugChar() != '+');
}

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

// Indicate that memory errors should be ignored
static volatile int catch_mem_err =0;

static void set_mem_fault_trap (int enable)
{
    catch_mem_err=enable;
}

/* Convert the memory pointed to by mem into hex, placing result in buf.
 * Return a pointer to the last char put in buf (null), in case of mem fault,
 * return 0.
 * If MAY_FAULT is non-zero, then we will handle memory faults by returning
 * a 0, else treat a fault like any other fault in the stub.
 */

static  char *mem2hex ( char *mem,  char *buf, int count, int may_fault)
{
  unsigned char ch;

  set_mem_fault_trap(may_fault);

  while (count-- > 0)
    {
      ch = *mem++;
      if (mem_err) return 0;
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch & 0xf];
    }

  *buf = 0;

  set_mem_fault_trap(0);

  return buf;
}

/* convert the hex array pointed to by buf into binary to be placed in mem
 * return a pointer to the character AFTER the last byte written */

static  char *hex2mem ( char *buf,  char *mem, int count, int may_fault)
{
  int i;
  unsigned char ch;

  set_mem_fault_trap(may_fault);

  for (i=0; i<count; i++)
    {
      ch = hex(*buf++) << 4;
      ch |= hex(*buf++);
      *mem++ = ch;
      if (mem_err)
    return 0;
    }

  set_mem_fault_trap(0);

  return mem;
}

/* This table contains the mapping between  hardware trap types, and
   signals, which are primarily what GDB understands.  It also indicates
   which hardware traps we need to commandeer when initializing the stub. */

static struct hard_trap_info
{
  uint32_t tt;     /* Trap type code */
  uint8_t signo;      /* Signal that we map this trap into */
} hard_trap_info[] = {
  {1, SIGSEGV},         /* instruction access error */
  {2, SIGILL},          /* privileged instruction */
  {3, SIGTRAP},          /* break*/
  {4, SIGBUS},          /* mem address not aligned */
  {6, SIGBUS},          /* mem address not aligned */
  {7, SIGBUS},          /* mem address not aligned */
  {9, SIGSEGV},         /* data access exception */
  {0, 0}            /* Must be last */
};



/* Convert the SPARC hardware trap type code to a unix signal number. */

static int
computeSignal (int tt)
{
  struct hard_trap_info *ht;

  for (ht = hard_trap_info; ht->tt && ht->signo; ht++)
    if (ht->tt == tt)
      return ht->signo;

  return SIGHUP;        /* default for things we don't know about */
}

/*
 * While we find nice hex chars, build an int.
 * Return number of chars processed.
 */

static int
hexToInt(char **ptr, int *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
    {
      hexValue = hex(**ptr);
      if (hexValue < 0)
    break;

      *intValue = (*intValue << 4) | hexValue;
      numChars ++;

      (*ptr)++;
    }

  return (numChars);
}


// For detection of nested calls....
static volatile int semaphore =0;

/*
 * This function does all command procesing for interfacing to gdb.
 */


static trapframe_t*  prepare_return(trapframe_t *ptf,int flagSingleStep)
{
  flush_cache();

  // In case of a hard coded ebreak jump over it
  if ( *((uint32_t*)ptf->epc)==0x00100073)  ptf->epc+=4;
  semaphore=0;
  catch_mem_err=0;
  PRINTK("prepare_return\n");

  if (flagSingleStep)  set_csr(0x7c0,1); // Set Single Step Mode
  asm("fence.i"); // Flush also instruction cache
  return ptf;
}


trapframe_t* handle_exception (trapframe_t *ptf)
{
  int tt;           /* Trap type */
  int sigval;
  int addr;
  int length;
  char *ptr;




  //if (semaphore) {
  //// We are in a nested call (most likely because of trying to step into code used by the debugger itself)
  //// In case of a ebreak jump over it

      //if ( *((uint32_t*)ptf->epc)==0x00100073)  ptf->epc+=4;
      //return ptf;
  //}



  clear_csr(0x7C0,1); // clear Single Step Mode

  //dump_tf(ptf);
  PRINTK("Trap %lx Semaphore %d\n",ptf->cause,semaphore);

  semaphore=1;
  ptf->gpr[0]=0;

  tt = ptf->cause;

  /* reply to host that an exception has occurred */
  sigval = computeSignal(tt);


  if (catch_mem_err && (sigval==SIGSEGV || sigval==SIGBUS)) {
    mem_err=1;
    ptf->epc+=4;
    return ptf;
  }

  ptr = remcomOutBuffer;

  *ptr++ = 'T';
  *ptr++ = hexchars[sigval >> 4];
  *ptr++ = hexchars[sigval & 0xf];

  *ptr++ = hexchars[RISCV_PC_REGNUM >> 4];
  *ptr++ = hexchars[RISCV_PC_REGNUM & 0xf];
  *ptr++ = ':';
  ptr=mem2hex((char*)&(ptf->epc),ptr,4,0);
  *ptr++ = ';';
  *ptr++ = 0;

  putpacket(remcomOutBuffer);

  while (1)
    {
      remcomOutBuffer[0] = 0;

      ptr = getpacket();
      PRINTK("< %s\n",ptr);
      if (strcmp(ptr,"qSupported")==0) {
        strcpy(remcomOutBuffer,"PacketSize=2048");
        putpacket(remcomOutBuffer);
        continue;
      }
      switch (*ptr++)
    {
    case '?':
      remcomOutBuffer[0] = 'S';
      remcomOutBuffer[1] = hexchars[sigval >> 4];
      remcomOutBuffer[2] = hexchars[sigval & 0xf];
      remcomOutBuffer[3] = 0;
      break;

    case 'd':       /* toggle debug flag */
      break;

    case 'g':       /* return the value of the CPU registers */
      {
        ptr = remcomOutBuffer;
        ptr=mem2hex((char*)&(ptf->gpr[0]),ptr,32*REGBYTES,0);
        ptr=mem2hex((char*)&(ptf->epc),ptr,REGBYTES,0);

      }
      break;

    case 'G':      /* set the value of the CPU registers - return OK */
      {
         if (hex2mem(ptr, (char *)&(ptf->gpr[0]),32*REGBYTES,0)) {
             strcpy(remcomOutBuffer, "OK");
          } else
             strcpy(remcomOutBuffer, "E03");

      }
      break;

    case 'p':

      if (hexToInt(&ptr, &addr)) {
        PRINTK("read register %lx\n",addr);
        if (addr>=0 && addr <=31)
           ptr=mem2hex((char*)&(ptf->gpr[addr]),remcomOutBuffer,REGBYTES,0);
        else {
          char *data=NULL;
          switch(addr) {
            case 32:
              data=(char*)&(ptf->epc);
              break;
            case CSR_MCAUSE:
              data=(char*)&(ptf->cause);
              break;
            default:
              data=(char*)&(ptf->gpr[0]);  // Dummy...
          }
          PRINTK("data : %lx\n",data);
          if (data) mem2hex(data,remcomOutBuffer,4,0);
        }
      }
      break;

   case 'P':
      if (hexToInt(&ptr, &addr)) {
         if (addr>=0 && addr <=31)
             hex2mem(ptr, (char *)&(ptf->gpr[addr]),REGBYTES,0);
          else
             strcpy(remcomOutBuffer, "E03");
      }
      break;

   case 'm':     /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      /* Try to read %x,%x.  */

      if (hexToInt(&ptr, &addr)
          && *ptr++ == ','
          && hexToInt(&ptr, &length))
        {
          if (mem2hex((char *)addr, remcomOutBuffer, length, 1))
             break;

          strcpy (remcomOutBuffer, "E03");
        }
      else
        strcpy(remcomOutBuffer,"E01");
      break;

    case 'M': /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
      /* Try to read '%x,%x:'.  */

      if (hexToInt(&ptr, &addr)
          && *ptr++ == ','
          && hexToInt(&ptr, &length)
          && *ptr++ == ':')
        {
          if (hex2mem(ptr, (char *)addr, length, 1)) {
             strcpy(remcomOutBuffer, "OK");

          } else
             strcpy(remcomOutBuffer, "E03");
        }
      else
        strcpy(remcomOutBuffer, "E02");
      break;

    case 'c':    /* cAA..AA    Continue at address AA..AA(optional) */
      /* try to read optional parameter, pc unchanged if no parm */

      if (hexToInt(&ptr, &addr))
      {
          ptf->epc=addr;
      }
      PRINTK("Continue at %08lx\n",ptf->epc);
      return prepare_return(ptf,0);

    case 's':
      PRINTK("step at %08lx\n",ptf->epc);
      return prepare_return(ptf,1);

      /* kill the program */
    case 'k' :

      ptf->epc=SRAM_BASE;
      semaphore=0;
      printk("MSTATUS: %lx\n",read_csr(mstatus));
      clear_csr(mstatus,MSTATUS_MPIE); // Global Interrupt Disable
      printk("MSTATUS: %lx\n",read_csr(mstatus));
      return ptf;


#if 0
    case 't':       /* Test feature */
      //asm (" std %f30,[%sp]");
      break;
#endif
    case 'r':       /* Reset */

      ptf->epc=SRAM_BASE;
      semaphore=0;
      clear_csr(mstatus,MSTATUS_MPIE); // Global Interrupt Disable
      return ptf;

    }           /* switch */

      /* reply to the request */
      putpacket(remcomOutBuffer);
    }
    return prepare_return(ptf,0);
}


trapframe_t* trap_handler(trapframe_t *ptf)
{

    if (ptf->cause & 0x80000000) {
      // place interrupt handler here...
      return ptf;
    }  else {
       return handle_exception(ptf);
    }
}




t_ptrapfuntion gdb_initDebugger(int set_mtvec)
{

  return handle_exception;
}




