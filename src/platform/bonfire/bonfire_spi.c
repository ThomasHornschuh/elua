#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"

#include "mem_rw.h"
#include "console.h"
#include <math.h>

#define PORT_OFFSET 64 // The register files of the bonfire-spi ports are aligned modulo 64

// Registers
/*
-- registers:
-- base+0   -- control register
--             Bit 0: Slave_cs (TODO: Check polarity...)
--             Bit 1: 1 = Autowait mode (Bus Blocks until transfer is finished), set by Default
-- base+4   -- status register
--                bit 0:  1 = "transfer in progress"
--                bit 1:  1 = RX data avaliable, will be cleared after reading RX register
-- base+8   -- transmitter: write a byte here, starts SPI bus transaction
-- base+0x0C   -- receiver: last byte received (updated on each transation)
-- base+0x10   -- transfer mode and clock register 
               --bit 7:0 clock divider: 
               --SPI CLK is spi_clk= clk/((divder+1)*2) ie for 100MHz clock, divisor 0 is 50MHz, 1 is 25MHz, 2 is 16,6MHz etc
               -- divider = (clk/(2*sclk))-1
               --bit 8: CPHA = clock phase
               --bit 9: CPOL = = clock polarity
*/

#define SPI_CONTROL          0
#define SPI_STATUS           4
#define SPI_TX               8
#define SPI_RX               0xc
#define SPI_MODE_DIVISOR     0x10

#define SPI_CPHA_BIT 8
#define SPI_CPOL_BIT 9


/*
 *  Driver for bonfire-spi core 
 *
 */

#define SPI_BASE_PTR   void *

 void * get_spi_base(unsigned id) {

   if (id>(NUM_SPI-1)) 
      return NULL;
   else    
     return (uint32_t*)(SPIFLASH_BASE+(PORT_OFFSET*id));
 }


int platform_spi_exists( unsigned id )
{
  return (id+1) <= NUM_SPI;
}

u32 platform_spi_setup( unsigned id, int mode, u32 clock, unsigned cpol, unsigned cpha, unsigned databits )
{
SPI_BASE_PTR base=get_spi_base(id);
double div;
uint32_t _div;

  if (base) {
      div = (double)SYSCLK/(double)(2*clock) - 1;
      printk("div: %f\n",div);
      if (div<0) {
        _div = 0;
      } else {
        _div = (div-floor(div))>0?(uint32_t)div + 1:(uint32_t)div;
        if (_div>255) _div=255;
      }
      printk("Set Clk divider to %ld\n",_div);
      _write_word(base+SPI_MODE_DIVISOR,_div | (cpol?1<<SPI_CPOL_BIT:0) | (cpha?1<<SPI_CPHA_BIT:0));
      return SYSCLK/((_div+1)*2);
  }
  return 0;
}

spi_data_type platform_spi_send_recv( unsigned id, spi_data_type data )
{
SPI_BASE_PTR base=get_spi_base(id);

  if (base) {
    while (_get_bit(base+SPI_STATUS,0)); // wait until a possible transfer is finished, just to be on the save side
    _write_word(base+SPI_TX,data);
    while (_get_bit(base+SPI_STATUS,0)); // wait until send is finished
    return _read_word(base+SPI_RX);
  }
  return 0;
}

void platform_spi_select( unsigned id, int is_select )
{
SPI_BASE_PTR base=get_spi_base(id);

  if (base) _write_word(base,is_select?0:1);
 
}
