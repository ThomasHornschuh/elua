#include "platform_conf.h"
#ifdef ELUA_PICOTCP_NG

#include "elua_net.h"
#include "pico_device.h"
#include "pico_ipv4.h"
#include "pico_socket.h"
#include "pico_stack.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PENDING 8
#define READ_BUFFER_SIZE 1460

typedef struct {
  int accept_request;
  struct pico_socket *sock;
  struct pico_ip4 remote;
} pending_accept_t;

static struct pico_stack *pico_stack;
static struct pico_device *pico_device;
static struct pico_ip4 local_address;
static pending_accept_t pending_accepts[MAX_PENDING];
static t_socketcallback socket_callback;
static int tick_semaphore;
static int last_error = ELUA_NET_ERR_OK;
static uintptr_t last_closed_socket;
static int last_closed_socket_bound;

/* The network device adapter is supplied by the selected platform.  The
 * simulator implementation uses Linux TAP; hardware platforms (including
 * Bonfire) can supply their own adapter without changing this wrapper. */
extern int elua_pico_device_send(struct pico_device *dev, void *buf, int len);
extern int elua_pico_device_poll(struct pico_device *dev, int loop_score);
extern void elua_pico_device_set_destroy(struct pico_device *dev);

/* PicoTCP-NG requires the embedding application to supply TCP sequence
 * randomness.  Keep this independent of the Linux TAP backend so a future
 * hardware device adapter can use the same stack wrapper. */
uint32_t pico_rand(void)
{
  static uint32_t state;

  if( state == 0 )
    state = (uint32_t)platform_timer_read_sys() ^
            (uint32_t)(uintptr_t)&state ^ 0x6d2b79f5u;
  state = state * 1664525u + 1013904223u;
  return state;
}

static int find_pending_accept(uint16_t port)
{
  int i;
  for( i = 0; i < MAX_PENDING; ++i )
    if( pending_accepts[i].accept_request &&
        pending_accepts[i].sock->local_port == short_be(port) )
      return i;
  return -1;
}

static void socket_event(uint16_t events, struct pico_socket *sock)
{
  if( events & PICO_SOCK_EV_CONN )
  {
    int i;
    for( i = 0; i < MAX_PENDING; ++i )
    {
      if( !pending_accepts[i].accept_request )
      {
        uint16_t port;
        struct pico_socket *accepted =
          pico_socket_accept(sock, &pending_accepts[i].remote, &port);
        if( accepted )
        {
          pending_accepts[i].sock = accepted;
          pending_accepts[i].accept_request = 1;
          if( socket_callback )
            socket_callback(ELUA_NET_CONNECT, (uintptr_t)sock);
        }
        return;
      }
    }
    fprintf(stderr, "PicoTCP-NG: incoming connection dropped (accept queue full)\n");
  }
  if( !socket_callback )
    return;
  if( events & PICO_SOCK_EV_RD )
    socket_callback(ELUA_NET_READ, (uintptr_t)sock);
  if( events & PICO_SOCK_EV_WR )
    socket_callback(ELUA_NET_WRITE, (uintptr_t)sock);
  if( events & PICO_SOCK_EV_FIN )
    socket_callback(ELUA_NET_FIN, (uintptr_t)sock);
  if( events & PICO_SOCK_EV_ERR )
    socket_callback(ELUA_NET_EVENT_ERR, (uintptr_t)sock);
  if( events & PICO_SOCK_EV_CLOSE )
  {
    pico_socket_shutdown(sock, PICO_SHUT_WR);
    socket_callback(ELUA_NET_CLOSE, (uintptr_t)sock);
  }
}

static void wait_for_stack(void)
{
  struct timespec delay = { 0, 1000000 };
  elua_pico_tick();
  nanosleep(&delay, NULL);
}

static int map_socket_option(int option)
{
  switch( option )
  {
    case PICO_TCP_NODELAY:
      return PICO_SOCKET_OPT_TCPNODELAY;
    case PICO_SOCKET_OPT_KEEPIDLE:
    case PICO_SOCKET_OPT_KEEPINTVL:
    case PICO_SOCKET_OPT_RCVBUF:
    case PICO_SOCKET_OPT_SNDBUF:
      return option;
    default:
      return -1;
  }
}

static void deinit_stack(void)
{
  heap_pico_timer_ref *timers;
  int i;

  if( !pico_stack )
    return;
  timers = pico_stack->Timers;
  pico_stack_deinit(pico_stack);
  if( timers )
  {
    for( i = 0; i < MAX_BLOCK_COUNT; ++i )
      if( timers->top[i] )
        PICO_FREE(timers->top[i]);
    PICO_FREE(timers);
  }
  PICO_FREE(pico_stack);
  pico_stack = NULL;
}

static int parse_ipv4(const char *text, struct pico_ip4 *address)
{
  return text && *text &&
         pico_string_to_ipv4(text, &address->addr) >= 0;
}

static int load_ipv4_config(const char *name, uint32_t *address,
                            uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  const char *override = getenv(name);
  uint8_t octets[4] = { a, b, c, d };
  struct pico_ip4 parsed;

  if( override )
  {
    if( !parse_ipv4(override, &parsed) )
    {
      fprintf(stderr, "PicoTCP-NG: invalid %s value '%s'\n", name, override);
      return -1;
    }
    *address = parsed.addr;
  }
  else
    memcpy(address, octets, sizeof(octets));
  return 0;
}

int elua_pico_init(void)
{
  struct pico_ip4 netmask;
  struct pico_ip4 gateway = { 0 };
  struct pico_ipv4_link *link;
  const char *gateway_override = getenv("ELUA_PICO_TAP_GATEWAY");
  int initialized_device;

  if( pico_stack_init(&pico_stack) != 0 || !pico_stack )
  {
    fprintf(stderr, "PicoTCP-NG: stack initialization failed\n");
    if( pico_stack )
      PICO_FREE(pico_stack);
    pico_stack = NULL;
    return -1;
  }

  if( load_ipv4_config("ELUA_PICO_TAP_IP", &local_address.addr,
                       ELUA_CONF_IPADDR0, ELUA_CONF_IPADDR1,
                       ELUA_CONF_IPADDR2, ELUA_CONF_IPADDR3) != 0 ||
      load_ipv4_config("ELUA_PICO_TAP_NETMASK", &netmask.addr,
                       ELUA_CONF_NETMASK0, ELUA_CONF_NETMASK1,
                       ELUA_CONF_NETMASK2, ELUA_CONF_NETMASK3) != 0 )
    goto fail;
  if( !pico_ipv4_is_unicast(local_address.addr) ||
      !pico_ipv4_valid_netmask(netmask.addr) )
  {
    fprintf(stderr, "PicoTCP-NG: TAP IPv4 address or netmask is invalid\n");
    goto fail;
  }

#ifdef ELUA_CONF_DEFGW0
  if( !gateway_override )
  {
    uint8_t octets[4] = { ELUA_CONF_DEFGW0, ELUA_CONF_DEFGW1,
                          ELUA_CONF_DEFGW2, ELUA_CONF_DEFGW3 };
    memcpy(&gateway.addr, octets, sizeof(octets));
    if( gateway.addr == 0 )
      gateway_override = "";
  }
#endif
  if( gateway_override && *gateway_override &&
      !parse_ipv4(gateway_override, &gateway) )
  {
    fprintf(stderr, "PicoTCP-NG: invalid ELUA_PICO_TAP_GATEWAY value '%s'\n",
            gateway_override);
    goto fail;
  }
  if( gateway.addr && !pico_ipv4_is_unicast(gateway.addr) )
  {
    fprintf(stderr, "PicoTCP-NG: TAP gateway must be a unicast IPv4 address\n");
    goto fail;
  }

  {
    const uint8_t *mac = platform_eth_init();
    if( !mac )
      goto fail;
    pico_device = PICO_ZALLOC(sizeof(*pico_device));
    if( !pico_device )
    {
      fprintf(stderr, "PicoTCP-NG: unable to allocate TAP device state\n");
      platform_eth_close();
      goto fail;
    }
    pico_device->send = elua_pico_device_send;
    pico_device->poll = elua_pico_device_poll;
    elua_pico_device_set_destroy(pico_device);
    if( pico_device_init(pico_stack, pico_device, getenv("ELUA_PICO_TAP"), mac) != 0 )
    {
      initialized_device = pico_get_device(pico_stack, getenv("ELUA_PICO_TAP")) ==
                           pico_device;
      if( !initialized_device )
      {
        platform_eth_close();
        PICO_FREE(pico_device);
        pico_device = NULL;
      }
      fprintf(stderr, "PicoTCP-NG: unable to initialize TAP device\n");
      goto fail;
    }
  }

  if( pico_ipv4_link_add(pico_stack, pico_device, local_address, netmask) != 0 )
  {
    fprintf(stderr, "PicoTCP-NG: unable to configure the TAP IPv4 address\n");
    goto fail;
  }
  link = pico_ipv4_link_by_dev(pico_stack, pico_device);
  if( !link )
  {
    fprintf(stderr, "PicoTCP-NG: TAP IPv4 link is unavailable\n");
    goto fail;
  }
  if( gateway.addr &&
      pico_ipv4_route_add(pico_stack, (struct pico_ip4){ 0 },
                          (struct pico_ip4){ 0 }, gateway, 1, link) != 0 )
  {
    fprintf(stderr, "PicoTCP-NG: unable to configure the TAP gateway route\n");
    goto fail;
  }
  return 0;

fail:
  deinit_stack();
  pico_device = NULL;
  return -1;
}

void elua_pico_shutdown(void)
{
  /* The simulator terminates immediately after this call.  PicoTCP-NG's
   * stack teardown can invoke device/socket callbacks after the Lua state has
   * gone away, so only release the host device here and let process exit
   * reclaim the stack.  A reusable hardware lifecycle needs its own ordered
   * shutdown path. */
  platform_eth_close();
  pico_device = NULL;
}

void elua_pico_tick(void)
{
  if( pico_stack && !tick_semaphore )
  {
    ++tick_semaphore;
    pico_stack_tick(pico_stack);
    --tick_semaphore;
  }
}

void elua_pico_unwind(void)
{
  tick_semaphore = 0;
}

void elua_pico_setdebug(bool enabled)
{
  (void)enabled;
}

int elua_pico_setdebug_output(const char *filename)
{
  (void)filename;
  return filename ? ENOTSUP : 0;
}

void elua_pico_set_socketcallback(t_socketcallback callback)
{
  socket_callback = callback;
}

uintptr_t elua_pico_getsocketoption(uintptr_t socket, int option)
{
  uintptr_t value = 0;
  int mapped_option = map_socket_option(option);
  return mapped_option >= 0 &&
         pico_socket_getoption((struct pico_socket *)socket, mapped_option, &value) == 0 ?
         value : (uintptr_t)-1;
}

int elua_pico_setsocketoption(uintptr_t socket, int option, int value)
{
  uintptr_t option_value = (uintptr_t)value;
  int mapped_option = map_socket_option(option);
  return mapped_option >= 0 ?
         pico_socket_setoption((struct pico_socket *)socket, mapped_option, &option_value) : -1;
}

uint32_t elua_pico_timer_add(uint64_t expire,
                             void (*timer)(uint64_t, void *), void *arg)
{
  return pico_stack ? pico_timer_add(pico_stack, expire, timer, arg) : 0;
}

void elua_pico_timer_cancel(uint32_t id)
{
  if( pico_stack )
    pico_timer_cancel(pico_stack, id);
}

dns_result_t *elua_net_lookup_async(const char *hostname, t_dnscallback callback)
{
  (void)hostname;
  (void)callback;
  return NULL;
}

int elua_pico_change_nameserver(elua_net_ip address, uint8_t flag)
{
  (void)address;
  (void)flag;
  return -1;
}

elua_net_ip elua_net_get_localip(void)
{
  elua_net_ip address;
  address.ipaddr = local_address.addr;
  return address;
}

int elua_pico_isbound(uintptr_t socket)
{
  return socket != 0 && socket == last_closed_socket && last_closed_socket_bound;
}

int elua_net_dhcp_state(void)
{
  return ELUA_DCHP_DISABLED;
}

elua_net_socket_t elua_net_socket(int type)
{
  struct pico_socket *sock;
  if( !pico_stack )
    return 0;
  sock = pico_socket_open(pico_stack, PICO_PROTO_IPV4,
                          type == ELUA_NET_SOCK_STREAM ? PICO_PROTO_TCP : PICO_PROTO_UDP,
                          socket_event);
  return (uintptr_t)sock;
}

int elua_net_close(elua_net_socket_t socket)
{
  struct pico_socket *sock = (struct pico_socket *)socket;
  int result;
  if( !sock )
    return ELUA_NET_ERR_INVALID_SOCKET;
  last_closed_socket = socket;
  last_closed_socket_bound = (sock->state & PICO_SOCKET_STATE_BOUND) != 0;
  result = pico_socket_close(sock);
  return result == 0 ? ELUA_NET_ERR_OK : ELUA_NET_ERR_INVALID_SOCKET;
}

elua_net_size elua_net_recvbuf(elua_net_socket_t socket, luaL_Buffer *buffer,
                               elua_net_size maxsize, s16 readto,
                               unsigned timer_id, timer_data_type timeout)
{
  timer_data_type started = timeout ? platform_timer_start(timer_id) : 0;
  struct pico_socket *sock = (struct pico_socket *)socket;
  elua_net_size received = 0;
  bool callback_mode = tick_semaphore != 0;
  char data[READ_BUFFER_SIZE];

  (void)readto;
  if( !sock )
    return 0;
  if( maxsize == 0 )
  {
    last_error = ELUA_NET_ERR_OK;
    return 0;
  }
  do
  {
    int len = pico_socket_read(sock, data,
                               (int)((maxsize - received) < sizeof(data) ?
                                     maxsize - received : sizeof(data)));
    if( len > 0 )
    {
      luaL_addlstring(buffer, data, (size_t)len);
      received += (elua_net_size)len;
    }
    else if( len < 0 )
    {
      last_error = ELUA_NET_ERR_ABORTED;
      return (elua_net_size)-1;
    }
    if( received || callback_mode )
    {
      last_error = ELUA_NET_ERR_OK;
      return received;
    }
    if( timeout == 0 ||
        (timeout != PLATFORM_TIMER_INF_TIMEOUT &&
         platform_timer_get_diff_crt(timer_id, started) >= timeout) )
    {
      last_error = ELUA_NET_ERR_WAIT_TIMEDOUT;
      return (elua_net_size)-1;
    }
    wait_for_stack();
  } while( received < maxsize );
  last_error = ELUA_NET_ERR_OK;
  return received;
}

elua_net_size elua_net_recv(elua_net_socket_t socket, void *buffer, elua_net_size maxsize,
                            s16 readto, unsigned timer_id, timer_data_type timeout)
{
  timer_data_type started = timeout ? platform_timer_start(timer_id) : 0;
  struct pico_socket *sock = (struct pico_socket *)socket;
  elua_net_size received = 0;
  bool callback_mode = tick_semaphore != 0;

  (void)readto;
  if( !sock )
    return 0;
  if( maxsize == 0 )
  {
    last_error = ELUA_NET_ERR_OK;
    return 0;
  }
  do
  {
    int len = pico_socket_read(sock, (char *)buffer + received,
                               (int)(maxsize - received));
    if( len > 0 )
    {
      received += (elua_net_size)len;
      last_error = ELUA_NET_ERR_OK;
      return received;
    }
    if( len < 0 )
    {
      last_error = ELUA_NET_ERR_ABORTED;
      return (elua_net_size)-1;
    }
    if( callback_mode )
    {
      last_error = ELUA_NET_ERR_OK;
      return 0;
    }
    if( timeout == 0 ||
        (timeout != PLATFORM_TIMER_INF_TIMEOUT &&
         platform_timer_get_diff_crt(timer_id, started) >= timeout) )
    {
      last_error = ELUA_NET_ERR_WAIT_TIMEDOUT;
      return (elua_net_size)-1;
    }
    wait_for_stack();
  } while( received < maxsize );
  last_error = ELUA_NET_ERR_OK;
  return received;
}

elua_net_size elua_net_send(elua_net_socket_t socket, const void *buffer, elua_net_size len)
{
  int result = pico_socket_write((struct pico_socket *)socket,
                                 buffer, (int)len);
  last_error = result < 0 ? ELUA_NET_ERR_ABORTED : ELUA_NET_ERR_OK;
  return result < 0 ? (elua_net_size)-1 : (elua_net_size)result;
}

elua_net_socket_t elua_accept(u16 port, unsigned timer_id, timer_data_type timeout,
                elua_net_ip *remote_ip)
{
  timer_data_type started = timeout ? platform_timer_start(timer_id) : 0;
  bool callback_mode = tick_semaphore != 0;
  int pending;

  do
  {
    pending = find_pending_accept(port);
    if( pending >= 0 )
    {
      remote_ip->ipaddr = pending_accepts[pending].remote.addr;
      pending_accepts[pending].accept_request = 0;
      last_error = ELUA_NET_ERR_OK;
      return (uintptr_t)pending_accepts[pending].sock;
    }
    if( callback_mode || timeout == 0 ||
        (timeout != PLATFORM_TIMER_INF_TIMEOUT &&
         platform_timer_get_diff_crt(timer_id, started) >= timeout) )
    {
      remote_ip->ipaddr = 0;
      last_error = ELUA_NET_ERR_WAIT_TIMEDOUT;
      return 0;
    }
    wait_for_stack();
  } while( 1 );
}

int elua_net_connect(elua_net_socket_t socket, elua_net_ip address, u16 port)
{
  int result = pico_socket_connect((struct pico_socket *)socket,
                                   &address.ipaddr, short_be(port));
  last_error = result < 0 ? ELUA_NET_ERR_ABORTED : ELUA_NET_ERR_OK;
  return result;
}

elua_net_ip elua_net_lookup(const char *hostname)
{
  elua_net_ip address = { 0 };
  struct pico_ip4 parsed;
  if( parse_ipv4(hostname, &parsed) )
    address.ipaddr = parsed.addr;
  return address;
}

int elua_listen(u16 port, BOOL listen)
{
  struct pico_socket *sock;
  struct pico_ip4 any = { 0 };
  uint16_t port_be = short_be(port);

  if( !listen || !port || !pico_stack )
    return -1;
  sock = pico_socket_open(pico_stack, PICO_PROTO_IPV4, PICO_PROTO_TCP, socket_event);
  if( !sock )
    return 0;
  if( pico_socket_bind(sock, &any, &port_be) != 0 ||
      pico_socket_listen(sock, MAX_PENDING) != 0 )
  {
    pico_socket_close(sock);
    return 0;
  }
  return (uintptr_t)sock;
}

int elua_net_get_last_err(elua_net_socket_t socket)
{
  (void)socket;
  return last_error;
}

int elua_net_get_telnet_socket(void)
{
  return 0;
}

#endif
