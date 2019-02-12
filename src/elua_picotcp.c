#include "platform_conf.h"
#ifdef BUILD_PICOTCP

#pragma message "Compiling elua_picotcp.c"

#include "platform.h"


#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dhcp_client.h"
#include "pico_ethernet.h"



#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static struct pico_device* dev;

volatile bool  dhcp_succes =false;

#include "console.h"
#define dbg printk 

void cb_dhcp(void *cli,int code)
{
struct pico_ip4 address, gw, netmask, dns;

char adr_b[16],gw_b[16],netmask_b[16],dns_b[16]; 

   dbg("DHCP callback %d\n",code);

   switch(code) {
     
     case PICO_DHCP_SUCCESS: 
        address=pico_dhcp_get_address(cli);
        gw=pico_dhcp_get_gateway(cli);
        netmask=pico_dhcp_get_netmask(cli);
        dns=pico_dhcp_get_nameserver(cli,0);

        pico_ipv4_to_string(adr_b,address.addr);
        pico_ipv4_to_string(gw_b,gw.addr);
        pico_ipv4_to_string(netmask_b,netmask.addr);
        pico_ipv4_to_string(dns_b,dns.addr);

        dbg("DHCP assigned  ip: %s mask: %s gw: %s dns: %s \n",
                adr_b,netmask_b,gw_b,dns_b); 

        dhcp_succes=true;

        break;
     case PICO_DHCP_ERROR: printf("DHCP Error\n"); break;

   }

}

extern struct pico_device *pico_eth_create(const char *name, const uint8_t *mac);

static bool initComplete  =false; 

void elua_pico_init( const uint8_t* paddr  )
{

struct pico_ip4 ipaddr, netmask;

uint32_t cid;

  pico_stack_init();
  dev = pico_eth_create( "eth0",paddr );
  /* assign the IP address to the  interface */
  pico_string_to_ipv4( "0.0.0.0", &ipaddr.addr );
  pico_string_to_ipv4( "255.255.255.0", &netmask.addr );
  pico_ipv4_link_add( dev, ipaddr, netmask );
  
  initComplete= true; 
  
  pico_dhcp_initiate_negotiation(dev,cb_dhcp,&cid);
  // Only for Debug purposes: Wait until DHCP success 
  while ( !dhcp_succes ){
       pico_stack_tick();
    };

}
void elua_pico_tick()
{   
  if (initComplete) pico_stack_tick();
}

#endif 