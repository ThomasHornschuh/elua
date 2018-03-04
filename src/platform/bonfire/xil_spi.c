#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"

#include "mem_rw.h"
#include "xspi_l.h"

#include "console.h"


// Switch on LOOP_BACK mode for debugging
//#define LOOP_BACK (XSP_CR_LOOPBACK_MASK)
// Switch off LOOP_BACK:
#define LOOP_BACK 0

/*
 *  Driver for Xilinx Quad SPI IP Core configured as "Standard" (1 Bit) SPI Interface
 *
 */


void* get_spi_base(unsigned id) {

  if (id>NUM_SPI-1) return 0;
  switch(id) {
    case 0:
      return (uint32_t*)AXI_QPSI0;
    default:
     return 0;
  }
}


int platform_spi_exists( unsigned id )
{
  return (id+1) <= NUM_SPI;
}

u32 platform_spi_setup( unsigned id, int mode, u32 clock, unsigned cpol, unsigned cpha, unsigned databits )
{
void *base=get_spi_base(id);

  //printk("spi_setup with id=%d mode=%d clock=%d cpol=%d cpha=%d databits=%d\n",id,mode,clock,cpol,cpha,databits);
  if ( base )  {
     _write_word(base+XSP_SRR_OFFSET,XSP_SRR_RESET_MASK); // RESET Controller
     _write_word(base+XSP_CR_OFFSET,XSP_CR_MANUAL_SS_MASK|
                      ((cpol<<3)&XSP_CR_CLK_POLARITY_MASK) |
                      ((cpha<<4)&XSP_CR_CLK_PHASE_MASK) |
                      XSP_CR_MASTER_MODE_MASK |
                      XSP_CR_ENABLE_MASK |
                      LOOP_BACK);
    return clock; // not really true....
  } else
    return 0;



}

spi_data_type platform_spi_send_recv( unsigned id, spi_data_type data )
{
void *base=get_spi_base(id);

  if ( base ) {
    while (_read_word(base+XSP_SR_OFFSET)&XSP_SR_TX_FULL_MASK); // Wait until TX FIFO has space
    _write_word(base+XSP_DTR_OFFSET,data);
    while (_read_word(base+XSP_SR_OFFSET)&XSP_SR_RX_EMPTY_MASK);
    return _read_word(base+XSP_DRR_OFFSET);

  } else
   return 0;

}

void platform_spi_select( unsigned id, int is_select )
{
void *base=get_spi_base(id);

  if ( base ) _write_word( base+ XSP_SSR_OFFSET, ~(is_select & 1));
}
