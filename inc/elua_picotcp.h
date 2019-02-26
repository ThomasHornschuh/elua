// picotcp "helper" for eLua


#ifndef __ELUA_PICOTCP_H__
#define __ELUA_PICOTCP_H__

typedef enum 
{
  ELUA_NET_CONNECT =0,
  ELUA_NET_READ,
  ELUA_NET_WRITE,
  ELUA_NET_CLOSE,
  ELUA_NET_FIN,
  ELUA_NET_EVENT_ERR

} t_socket_event;


typedef void (*t_socketcallback)(t_socket_event ev, uintptr_t socket);

void elua_pico_init();
void elua_pico_tick();
void elua_pico_unwind();

uintptr_t elua_pico_getsocketoption(uintptr_t socket, int option);

void elua_pico_set_socketcallback(t_socketcallback cb);

#endif 