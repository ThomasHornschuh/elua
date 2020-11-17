// "CPU" Description for Bonfire SoC Version 1.0 on Papilio Pro board

#ifndef CPU_BONFIRE_BASIC_SOC_10__H__
#define CPU_BONFIRE_BASIC_SOC_10__H__

#include "bonfire_basic_soc.h"
#include "stacks.h"

#ifndef __ASSEMBLER__
#include "platform_ints.h"
#endif

#define CPU_FREQUENCY SYSCLK

#define PLATFORM_HAS_SYSTIMER

// Number of resources (0 if not available/not implemented)
#define NUM_PIO               1
#define NUM_SPI               3 // TODO: Make configurable
//#define NUM_UART              1
#define NUM_TIMER             1
#define NUM_PWM               0
#define NUM_ADC               0
#define NUM_CAN               0
#define NUM_I2C               1


// PIO prefix ('0' for P0, P1, ... or 'A' for PA, PB, ...)
#define PIO_PREFIX            'A'
// Pins per port configuration:
// #define PIO_PINS_PER_PORT (n) if each port has the same number of pins, or
// #define PIO_PIN_ARRAY { n1, n2, ... } to define pins per port in an array
// Use #define PIO_PINS_PER_PORT 0 if this isn't needed
#define PIO_PIN_ARRAY {8}  // {8,4,8}

#define PORT_SHIFT {0}   //{0,8,12}

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

