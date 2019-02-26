#include "platform_conf.h"
#ifdef BUILD_PICOTCP

#pragma message "Compiling elua_picotcp.c"

#include "platform.h"
#include "elua_net.h"


#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dhcp_client.h"
#include "pico_ethernet.h"
#include "pico_socket.h"
#include "pico_dns_client.h"



#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static struct pico_device* dev;

static volatile int  dhcp_state = ELUA_DHCP_UNCONFIGURED;


static volatile bool debug_enabled = true; 

static volatile int last_error = ELUA_NET_ERR_OK;

static volatile int tick_semaphore = 0; 


// typedef struct {
//   struct pico_socket* s;
//   uint16_t event;


// } t_socket;

typedef struct {
  u8 accept_request; // 0=accepted/not used 1=pending
  struct pico_socket* sock;   // socket  when accepted
  struct pico_ip4 remote; // remote IP
  int port; // remote port bound to

} elua_uip_accept_pending_t;

#define MAX_PENDING 8 // TODO...

static elua_uip_accept_pending_t elua_uip_accept_pending[MAX_PENDING];

static t_socketcallback  socket_cb = NULL;

void platform_debug_printk(const char* s, ...)
{
va_list args;

  if (debug_enabled) {
    va_start (args, s);
    vprintf (s, args);
    va_end (args);    
  }
}



void elua_pico_set_socketcallback(t_socketcallback cb)
{
  socket_cb=cb;
}

void elua_net_set_debug_log(int enable)
{
  debug_enabled= enable;
}

int elua_net_dhcp_state()
{
    return dhcp_state;
}

static int elua_net_find_pending( u16 port )
{
int i;


  for( i=0;i<MAX_PENDING;i++ )
  {
    if ( elua_uip_accept_pending[i].accept_request==1
        && elua_uip_accept_pending[i].sock->local_port==short_be(port) )
      return i;
  }
  return -1;
}

static inline void  __cb(t_socket_event ev, struct pico_socket *s)
{
  if (socket_cb) socket_cb(ev,(uintptr_t)s); 
}

void cb_socket(uint16_t ev, struct pico_socket *s)
{
   if (ev & PICO_SOCK_EV_CONN) {
     // Add to list of pending connections 
     int i;
     BOOL found=FALSE;
     uint16_t port;
     for( i=0;i<MAX_PENDING && !found;i++ )
     {
        if ( elua_uip_accept_pending[i].accept_request!=1 ) {// free slot
          elua_uip_accept_pending[i].sock=pico_socket_accept(s, &elua_uip_accept_pending[i].remote, &port);
          elua_uip_accept_pending[i].port=short_be(port);
          
          elua_uip_accept_pending[i].accept_request=1;
          found=TRUE;
        }
     }
     if ( !found ) { // no free slot
       platform_debug_printk("Accept event without free pending slots\n");
       //TODO: How to deal with this situation? 
     } else {
       __cb(ELUA_NET_CONNECT,s);
     }
   }

   if (ev & PICO_SOCK_EV_RD) {
      __cb(ELUA_NET_READ,s);
   }
    if (ev & PICO_SOCK_EV_WR) {
      __cb(ELUA_NET_WRITE,s);
   }

   /* process fin event, receiving socket closed */
    if (ev & PICO_SOCK_EV_FIN)
    {
        platform_debug_printk("Socket closed. Exit normally. \n");
        __cb(ELUA_NET_FIN,s);
    }
    /* process error event, socket error occured */
    if (ev & PICO_SOCK_EV_ERR)
    {
        platform_debug_printk("Socket Error received: %s. Bailing out.\n", strerror(pico_err));
        __cb(ELUA_NET_EVENT_ERR,s);
       
    }
    /* process close event, receiving socket received close from peer */
    if (ev & PICO_SOCK_EV_CLOSE)
    {
        platform_debug_printk("Socket received close from peer.\n");
        /* shutdown write side of socket */
        pico_socket_shutdown(s, PICO_SHUT_WR);
        __cb(ELUA_NET_CLOSE,s);
    }
}


// static t_socket * create_socket_obj(struct pico_socket* ps)
// {
// t_socket * s=malloc( sizeof(t_socket) );

//      s->s = ps;
//      s->event=0; 
//      return s; 
// }

int elua_net_socket( int type )
{
struct pico_socket * ps = pico_socket_open(PICO_PROTO_IPV4,
                                          (type==ELUA_NET_SOCK_STREAM?PICO_PROTO_TCP:PICO_PROTO_UDP),
                                          &cb_socket);  
   if ( ps ) {   
     return (int) ps;
   } else
   {
     return 0;
   }
                                    

}
int elua_net_close( int s )
{   
  return  pico_socket_close((struct pico_socket*)s )==0?ELUA_NET_ERR_OK:ELUA_NET_ERR_INVALID_SOCKET; 
}

#define BSIZE 1460

static inline int  _min(int a,int b)
{
   return a<b?a:b;
}

elua_net_size elua_net_recvbuf( int s, luaL_Buffer *buf, elua_net_size maxsize, s16 readto, unsigned timer_id, timer_data_type to_us )
{
timer_data_type tmrstart = 0;
bool cb_mode= tick_semaphore != 0; // recv called a pico_tick callback?


    if (s==0 || s== -1 ) return 0;
    struct pico_socket* ps =  (struct pico_socket*)s;
    
    if( to_us > 0 )
      tmrstart = platform_timer_start( timer_id );

    while (1) {
      if (!cb_mode) pico_stack_tick();
      //if (ps->ev_pending & PICO_SOCK_EV_RD) {
         char recvbuf[BSIZE];
         int read = 0;
         int len = 0;
         do  {
            
            read = pico_socket_read(ps, recvbuf, _min(BSIZE,maxsize-len));
            if (read > 0) {
                len += read;
                luaL_addlstring(buf,recvbuf,read);
            }    
        } while (read > 0);
        if (read<0) {
           last_error = ELUA_NET_ERR_ABORTED;
           return -1;
        }   
        if (len>0 || cb_mode) {
            last_error = ELUA_NET_ERR_OK;
            return len;
        }    
      //}
      if( to_us == 0 || platform_timer_get_diff_crt( timer_id, tmrstart ) >= to_us )
      {
        last_error = ELUA_NET_ERR_WAIT_TIMEDOUT;
        return -1;
      }
    }
}


elua_net_size elua_net_recv( int s, void *buf, elua_net_size maxsize, s16 readto, unsigned timer_id, timer_data_type to_us )
{
  return 0;
}


elua_net_size elua_net_send( int s, const void* buf, elua_net_size len )
{
int r;  
   if (s==0 || s== -1 ) return -1;

   r = pico_socket_send((struct pico_socket*)s,buf,len);
   if ( r== -1 ) {
     last_error = ELUA_NET_ERR_ABORTED;
     return r;
   } else { 
     last_error = ELUA_NET_ERR_OK;
     return len;
   }
}
int elua_accept( u16 port, unsigned timer_id, timer_data_type to_us, elua_net_ip* pfrom )
{
timer_data_type tmrstart = 0;
bool cb_mode= tick_semaphore != 0; // recv called a pico_tick callback?

  int i;

 
  if( to_us > 0 )
    tmrstart = platform_timer_start( timer_id );
  while( 1 )
  {
    if (!cb_mode) pico_stack_tick();
    i=elua_net_find_pending( port );
    if( i >= 0 ) {
      pfrom->ipaddr = elua_uip_accept_pending[i].remote.addr;
      elua_uip_accept_pending[i].accept_request=0;
      last_error = ELUA_NET_ERR_OK;
      return (int)elua_uip_accept_pending[i].sock;
    } 
    if (cb_mode) {
      last_error = ELUA_NET_ERR_OK;
      return 0;
    } else if( to_us == 0 || platform_timer_get_diff_crt( timer_id, tmrstart ) >= to_us )
    {
      last_error = ELUA_NET_ERR_WAIT_TIMEDOUT;
      pfrom->ipaddr=0;
      return 0;
    }
  }
}



int elua_net_connect( int s, elua_net_ip addr, u16 port )
{
  int res= pico_socket_connect((struct pico_socket *)s,(void*)&addr.ipaddr,short_be(port) ) ;
  last_error = (res<0)?ELUA_NET_ERR_ABORTED:ELUA_NET_ERR_OK;

  return res; 
  
}


typedef struct {
  uint32_t ipaddr;
  bool success;
} dns_result_t;

static void cb_dns_getaddr(char *ip, void *arg)
{
   

   dns_result_t * r = (dns_result_t * ) arg  ;
   if(ip) {
      pico_string_to_ipv4(ip, &r->ipaddr);
   }
  
   r->success=true; 

}


elua_net_ip elua_net_lookup( const char* hostname )
{
dns_result_t r = { 0,false };

elua_net_ip ip = {0};

  

  if( pico_dns_client_getaddr(hostname,&cb_dns_getaddr,(void*)&r) ==0 ) {
    while (!r.success) {
      pico_stack_tick();
    }
    ip.ipaddr=r.ipaddr;
  }
  return ip;
}

int elua_listen( u16 port, BOOL flisten )
{
struct pico_socket *s;
uint16_t port_be = 0;
int backlog = MAX_PENDING; /* max number of accepting connections */
struct pico_ip4 inaddr_any;
int ret;

    if (!flisten || port==0 ) return -1;
    /* set the source port for the socket */
   
    port_be = short_be(port);
    /* open a TCP socket with the appropriate callback */
    s = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &cb_socket);
    if (!s)
        return 0;
    /* bind the socket to port_be */
    inaddr_any.addr=0;
    ret = pico_socket_bind(s, &inaddr_any, &port_be);
    // TODO: Avoid Memory leak - how to delete s?
    if (ret != 0) return 0;
        
    /* start listening on socket */
    ret = pico_socket_listen(s, backlog);
  
    return ret!=0?0:(int)s;

}

int elua_net_get_last_err( int s )
{
  return (int) last_error;
}


int elua_net_get_telnet_socket( void )
{
  return 0;

}

uintptr_t elua_pico_getsocketoption(uintptr_t socket, int option)
{
uintptr_t optvalue;

  if (pico_socket_getoption((struct pico_socket*)socket,option,&optvalue)==0)
    return optvalue;
  else
    return -1;    
}


static void cb_dhcp(void *cli,int code)
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

        platform_debug_printk("DHCP assigned  ip: %s mask: %s gw: %s dns: %s  Hostname: %s\n",
                               adr_b,netmask_b,gw_b,dns_b,pico_dhcp_get_hostname()); 

        dhcp_state=ELUA_DHCP_SUCCESS;

        break;
     case PICO_DHCP_ERROR: 
        dhcp_state=ELUA_DCHP_FAILURE;
        platform_debug_printk("DHCP Error\n"); 
        break;

   }

}

extern struct pico_device *pico_eth_create(const char *name);

static bool initComplete  =false; 

void elua_pico_init()
{

struct pico_ip4 ipaddr, netmask;

uint32_t cid;

  pico_stack_init();
  dev = pico_eth_create( "eth0");
  /* assign the IP address to the  interface */
  pico_string_to_ipv4( "0.0.0.0", &ipaddr.addr );
  pico_string_to_ipv4( "255.255.255.0", &netmask.addr );
  pico_ipv4_link_add( dev, ipaddr, netmask );
  
  initComplete= true; 
 
  pico_dhcp_initiate_negotiation(dev,cb_dhcp,&cid);
  while ( dhcp_state==ELUA_DHCP_UNCONFIGURED ) {
    pico_stack_tick();
  };
  debug_enabled = false; // Disable debug log after inital sequence to avoid screen clobbering 

}



void elua_pico_tick()
{   
  if (initComplete) {
    if (tick_semaphore) return; // avoid recursive pico tick calls
    tick_semaphore++; 
    pico_stack_tick();
    tick_semaphore--;
    
  }  
}

void elua_pico_unwind()
{
  tick_semaphore--;
}

#endif 