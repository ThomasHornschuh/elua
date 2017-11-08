#ifndef PAPILIO_PRO_H
#define PAPILIO_PRO_H

#pragma message "PAPILIO_PRO_H"

// New Defintions for new bonfire-soc-io core
#define IO_BASE 0x04000000
#define SOC_IO_OFFSET 0x10000 // Offset from one I/O Device to the next (64K range)

#define UART0_BASE IO_BASE
#define SPIFLASH_BASE (IO_BASE+SOC_IO_OFFSET)
#define GPIO_BASE (IO_BASE+3*SOC_IO_OFFSET)
#define UART1_BASE (IO_BASE+2*SOC_IO_OFFSET)

#define UART_BASE UART0_BASE // Backwards compatiblity

#define MTIME_BASE 0x0FFFF0000

#define DRAM_BASE 0x0
#define DRAM_SIZE (8192*1024) // 8 Megabytes
#define DRAM_TOP  (DRAM_BASE+DRAM_SIZE-1)

#define SRAM_BASE 0x0C000000
#define SRAM_SIZE 32768
#define SRAM_TOP  (SRAM_BASE+SRAM_SIZE-1)


#define SYSCLK 96000000

#define CLK_PERIOD (1e+9 / SYSCLK)  // in ns...


// Parameters for SPI Flash

#define FLASHSIZE (8192*1024)
#define MAX_FLASH_IMAGESIZE (1024*512) // Max 512KB of flash used for boot image
#define FLASH_IMAGEBASE (1024*512)  // Boot Image starts at 512KB in Flash

#endif
