#ifndef BONFIRE_BASIC_SOC_H
#define BONFIRE_BASIC_SOC_H

//#pragma message "BONFIRE_BASIC_SOC_H"

// New Defintions for new bonfire-soc-io core
#define IO_BASE 0x04000000
#define SOC_IO_OFFSET 0x10000 // Offset from one I/O Device to the next (64K range)

#define UART0_BASE IO_BASE
#define SPIFLASH_BASE (IO_BASE+SOC_IO_OFFSET)
#define GPIO_BASE (IO_BASE+3*SOC_IO_OFFSET)
#define UART1_BASE (IO_BASE+2*SOC_IO_OFFSET)

#define UART_BASE UART0_BASE // Backwards compatiblity


// Interrupts

#define UART0_INTNUM (16+6) // Assume UART0 is assigned to LI6
#define UART1_INTNUM (16+5) // Assume UART1 is assigned to LI5


#define MTIME_BASE 0x0FFFF0000

#define DRAM_BASE 0x0
#define DRAM_SIZE (32768*1024) // 32 Megabytes
#define DRAM_TOP  (DRAM_BASE+DRAM_SIZE-1)

#define SRAM_BASE 0xC0000000
#define SRAM_SIZE 16384
#define SRAM_TOP  (SRAM_BASE+SRAM_SIZE-1)

//#define DCACHE_SIZE (2048*4)
#define DCACHE_SIZE   (2048*4)  // DCache Size in Bytes

#ifndef SYSCLK
#define SYSCLK 62500000 // 62.5 MHz 
#endif

#define CLK_PERIOD (1e+9 / SYSCLK)  // in ns...


// Parameters for SPI Flash

#define FLASHSIZE (8192*1024)
#define MAX_FLASH_IMAGESIZE (2024*1024) // Max 2MB of flash used for boot image
#define FLASH_IMAGEBASE (1024*6144)  // Boot Image starts at 6MB in Flash



#endif
