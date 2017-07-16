#ifndef __ARTY_H
#define __ARTY_H

#pragma message "arty.h"

#define WISHBONE_IO_SPACE 0x40000000
#define AXI_IO_SPACE 0x80000000

#define UART_BASE (WISHBONE_IO_SPACE)
#define SPIFLASH_BASE (WISHBONE_IO_SPACE+0x100)
#define SYSIO_BASE (WISHBONE_IO_SPACE+0x200)
#define GPIO_BASE (AXI_IO_SPACE)
#define MTIME_BASE 0x0FFFF0000


#define DRAM_BASE 0x0
#define DRAM_TOP  0x0fffffff
#define DRAM_SIZE (DRAM_TOP-DRAM_BASE+1)
#define SRAM_BASE 0xC0000000
#define SRAM_SIZE (32*1024)
#define SRAM_TOP  (SRAM_BASE+SRAM_SIZE-1)

#define SYSCLK 83333333  

#define CLK_PERIOD (1e+9 / SYSCLK)  // in ns...


// Parameters for SPI Flash 

#define FLASHSIZE (16384*1024)
#define MAX_FLASH_IMAGESIZE (2024*1024) // Max 2MB of flash used for boot image
#define FLASH_IMAGEBASE (1024*3072)  // Boot Image starts at 3MB in Flash




#endif
