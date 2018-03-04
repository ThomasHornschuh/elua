// "CPU" Description for Bonfire SoC Version 1.0 on ARTY Board

#ifndef __CPU_BONFIRE_ARTY_10_H__
#define __CPU_BONFIRE_ARTY_10_H__

#define BONFIRE_ARTY_10

#include "arty.h"
#include "stacks.h"

#ifndef __ASSEMBLER__
#include "platform_ints.h"
#endif

#define CPU_FREQUENCY SYSCLK

#define PLATFORM_HAS_SYSTIMER

// Number of resources (0 if not available/not implemented)
#define NUM_PIO               0
#define NUM_SPI               1
#define NUM_UART              1
#define NUM_TIMER             1
#define NUM_PWM               0
#define NUM_ADC               0
#define NUM_CAN               0


// PIO prefix ('0' for P0, P1, ... or 'A' for PA, PB, ...)
#define PIO_PREFIX            'A'
// Pins per port configuration:
// #define PIO_PINS_PER_PORT (n) if each port has the same number of pins, or
// #define PIO_PIN_ARRAY { n1, n2, ... } to define pins per port in an array
// Use #define PIO_PINS_PER_PORT 0 if this isn't needed
#define PIO_PINS_PER_PORT     0

//// Allocator data: define your free memory zones here in two arrays
//// (start address and end address)
//extern void *memory_start_address;
//extern void *memory_end_address;
//#define MEM_LENGTH (4000*1024) // 4MB
//#define INTERNAL_RAM1_FIRST_FREE ( void* )memory_start_address
//#define INTERNAL_RAM1_LAST_FREE  ( void* )memory_end_address

//extern  char  _endofheap; // Set by Linker script

#define INTERNAL_RAM1_FIRST_FREE (  end )
#define INTERNAL_RAM1_LAST_FREE  ( DRAM_TOP - STACK_SIZE_TOTAL )



#endif

