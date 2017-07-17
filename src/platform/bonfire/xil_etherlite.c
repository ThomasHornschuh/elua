/*
 * eLua Platform network interface for Xilinx AXI Ethernet Lite MAC
 * (c) 2017 Thomas Hornschuh
 *
 */


#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "console.h"
#include "xil_etherlite.h"
#include "irq_handler.h"
#include "uip.h"
#include "elua_uip.h"

#include "encoding.h"
#include <string.h>


#define ETH_BUFSIZE_WORDS (0x07f0/4)

inline void _write_word(void* address,uint32_t value)
{
  *((uint32_t*)(address))=value;
}


uint32_t _read_word(void* address)
{
  return  *((uint32_t*)(address));

}


void platform_eth_init()
{
// {0,0,0x5E,0,0xFA,0xCE}
static struct uip_eth_addr sTempAddr = {
    .addr[0] = 0,
    .addr[1] = 0,
    .addr[2] = 0x5e,
    .addr[3] = 0,
    .addr[4] = 0x0fa,
    .addr[5] = 0x0ce
  };

  printk("Initalizing Ethernet core\n");
  _write_word(ETHL_GIE,0x80000000); // Enable Interrupts
  set_csr(mie,MIP_MEIP); // Enable External Interrupt


  // clear pending packets
  _write_word(ETHL_RX_PING_CTRL,0x8);
  _write_word(ETHL_RX_PONG_CTRL,0x8);
  _write_word(ETHL_TX_PING_CTRL,0);


   elua_uip_init( &sTempAddr );


}


// Copy from to Buffers. Because the Etherlite core don't
// support byte writes the code ensures to do 32 Bit transfers
void copy_buffer(void* dest, const void* src, size_t size)
{
const uint32_t *psrc = src;
uint32_t *pdest = dest;

size_t szwords =(size % 4)?size/4+1:size/4; // round up
int i;

    //printk("copy from %x to %x, %d bytes, %d words\n",src,dest,size,szwords);
    if (szwords>ETH_BUFSIZE_WORDS) {
      do_panic("panic: Ethernet buffer copy size overflow: %d\n",szwords);
    }
    for(i=0;i<szwords;i++)
       pdest[i]=psrc[i];
}


void platform_eth_send_packet( const void* src, u32 size )
{
   //printk("send packet\n");
   while (_read_word(ETHL_TX_PING_CTRL) & 0x01); // Wait until buffer ready

   copy_buffer(ETHL_TX_PING_BUFF,src,size);
   _write_word(ETHL_TX_PING_LEN,size);
   _write_word(ETHL_TX_PING_CTRL,0x01); // Start send
   //printk("packet send started\n");

}


u32 platform_eth_get_packet_nb( void* buf, u32 maxlen )
{
static BOOL is_PingBuff = true;      // start always with the ping buffer
void * currentBuff;
int i;

  if (is_PingBuff)
    currentBuff= ETHL_RX_PING_BUFF;
  else
    currentBuff= ETHL_RX_PONG_BUFF;


  if (_read_word(currentBuff+0x7fc) & 0x01) {
     //copy_buffer(buf,currentBuff,maxlen);
     memcpy(buf,currentBuff,maxlen);
     _write_word(currentBuff+0x7fc,0x8); // clear buffer, enable interrupts
     //for(i=0;i<16;i++) printk("%x ",((uint8_t*)buf)[i]);
     //printk("\n");
     is_PingBuff = !is_PingBuff; // toggle
     return maxlen;

  } else
    return 0;

}
void platform_eth_force_interrupt(void)
{
  elua_uip_mainloop();
}

u32 platform_eth_get_elapsed_time(void)
{

    if( eth_timer_fired )
    {
      eth_timer_fired = 0;
      return __virt_timer_period;
    }
    else
      return 0;

}



void ethernet_irq_handler()
{
   if (_read_word((void*)BONFIRE_SYSIO) & 0x01) { // Pending IRQ
      _write_word((void*)BONFIRE_SYSIO,0x01); // clear IRQ
#ifdef  BUILD_UIP
     // printk("ethernet_irq_handler\n");
      elua_uip_mainloop();

#endif
   } else
     printk("Uups, ethernet irq handler called without pending IRQ\n");


}

void uip_log(char *msg)
{

 // printk("UIP Log %s\n",msg);
}
