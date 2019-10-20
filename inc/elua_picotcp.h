// picotcp "helper" for eLua


#ifndef __ELUA_PICOTCP_H__
#define __ELUA_PICOTCP_H__



#include <stdint.h>
#include <stdbool.h>

typedef enum
{
  ELUA_NET_CONNECT =0,
  ELUA_NET_READ,
  ELUA_NET_WRITE,
  ELUA_NET_CLOSE,
  ELUA_NET_FIN,
  ELUA_NET_EVENT_ERR

} t_socket_event;


typedef struct _dns_result dns_result_t; // forward

typedef void (*t_dnscallback)( dns_result_t *dns_result );

typedef struct  _dns_result {
  uint32_t ipaddr;
  bool success;
  t_dnscallback cb;
} dns_result_t;



typedef void (*t_socketcallback)(t_socket_event ev, uintptr_t socket);




void elua_pico_init();
void elua_pico_tick();
void elua_pico_unwind();
void elua_pico_setdebug(bool d);
int elua_pico_setdebug_output(const char* filename);

uintptr_t elua_pico_getsocketoption( uintptr_t socket, int option );
int  elua_pico_setsocketoption( uintptr_t socket, int option, int value );

void elua_pico_set_socketcallback( t_socketcallback cb );

dns_result_t  * elua_net_lookup_async( const char* hostname, t_dnscallback cb  );

int elua_pico_change_nameserver( elua_net_ip addr, uint8_t flag );

elua_net_ip elua_net_get_localip();

int elua_pico_isbound(uintptr_t socket);

#endif
