// Platform-dependent functions

#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"
#include <reent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "term.h"
#include "console.h"
#include "common.h"

#include "uart.h"
#include "irq_handler.h"
#include "encoding.h"

#include <stdio.h>

// ****************************************************************************
// Terminal support code

#ifdef BUILD_TERM

static void i386_term_out( u8 data )
{
 uart_writechar(data);
}

static int i386_term_in( int mode )
{
  if( mode == TERM_INPUT_DONT_WAIT )
    return -1;
  else
   return uart_readchar();
}

static int i386_term_translate( int data )
{
  int newdata = data;

  if( data == 0 )
    return KC_UNKNOWN;
  else switch( data )
  {
    case '\n':
      newdata = KC_ENTER;
      break;

    case '\t':
      newdata = KC_TAB;
      break;

    case '\b':
      newdata = KC_BACKSPACE;
      break;

    case 0x1B:
      newdata = KC_ESC;
      break;
  }
  return newdata;
}

#endif // #ifdef BUILD_TERM

// *****************************************************************************
// std functions
static void scr_write( int fd, char c )
{
    if (c=='\n') uart_writechar('\r');   
    uart_writechar(c);
}

static int kb_read( timer_data_type to )
{

  if( to != STD_INFINITE_TIMEOUT )
    return -1;
  else
  {
    
    return uart_readchar();
  }
}


trapframe_t* trap_handler(trapframe_t *ptf)
{
char c;
   
    if (ptf->cause & 0x80000000) {
       // interrupt   
       if ((ptf->cause & 0x0ff)==7)
         timer_irq_handler();
       else 
         printk("Unexepted Interrupt cause %x\n",ptf->cause);    
        
      // interrupts must not increment epc !!!
        
    }  else {  
     // trap   
  
		printk("\nTrap cause: %lx\n",ptf->cause);
		dump_tf(ptf);
		c=uart_readchar();
		if (c=='r' || c=='R')
		  ptf->epc=SRAM_BASE; // will cause reset
		else
		  ptf->epc+=4;
        
    }    
    return ptf;
}


// ****************************************************************************
// Platform initialization (low-level and full)

void *memory_start_address = 0;
void *memory_end_address = 0;

extern char end; // From linker script, end of data segment
extern char _endofheap; // From Linker script



void platform_ll_init( void )
{

	
  uart_write_console("platform_ll_init\n");	
	   
}

int platform_init()
{ 
 
  uart_write_console("platform_init\n");	
  cmn_systimer_set_base_freq(SYSCLK);
  cmn_systimer_set_interrupt_period_us(0x0ffffffff/SYSCLK *1000000);
  
  set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable
  platform_timer_sys_enable_int();
 
 

  // Set the std input/output functions
  // Set the send/recv functions                          
  std_set_send_func( scr_write );
  std_set_get_func( kb_read );       

  // Set term functions
#ifdef BUILD_TERM  
  term_init( TERM_LINES, TERM_COLS, i386_term_out, i386_term_in, i386_term_translate );
#endif

  //term_clrscr();
  //term_gotoxy( 1, 1 );
  
//  uart_write_console(" calling cmn_platform_init\n");	
  
 // cmn_platform_init(); // call the common initialiation code
 
 //  uart_write_console(" return from cmn_platform_init\n");	
  // All done
  return PLATFORM_OK;
}

// ****************************************************************************
// "Dummy" UART functions

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{
  uart_setBaudRate(baud);
  return baud;
}

void platform_s_uart_send( unsigned id, u8 data )
{
	uart_writechar(data);
}

int platform_s_uart_recv( unsigned id, timer_data_type timeout )
{
  return uart_wait_receive(timeout);
}

int platform_s_uart_set_flow_control( unsigned id, int type )
{
  return PLATFORM_ERR;
}

// ****************************************************************************
// "Dummy" timer functions

void platform_s_timer_delay( unsigned id, timer_data_type delay_us )
{
}

timer_data_type platform_s_timer_op( unsigned id, int op, timer_data_type data )
{
  return 0;
}

timer_data_type platform_timer_read_sys( void )
{
  return cmn_systimer_get();
}

// ****************************************************************************
// "Dummy" CPU functions


int platform_cpu_set_global_interrupts( int status )
{
  
  if (status==PLATFORM_CPU_ENABLE)
    set_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable	
  else
    clear_csr(mstatus,MSTATUS_MIE); // Global Interrupt Enable	
    
  return PLATFORM_OK;
}

int platform_cpu_get_global_interrupts( void )
{
uint32_t m = read_csr(mstatus);
   
   if (m & MSTATUS_MIE)
     return 1;
   else  	
    return 0;
}

