#define _GNU_SOURCE
#include "platform_conf.h"
#ifdef ELUA_PICOTCP_NG

#include "platform.h"
#include "pico_device.h"
#include "pico_eth.h"
#include "pico_stack.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define SIM_ETH_MAX_FRAME (PICO_ETH_MRU)

static int tap_fd = -1;
static char tap_name[IFNAMSIZ];
static uint8_t device_mac[6];

const u8 *platform_eth_init(void)
{
  const char *selected = getenv("ELUA_PICO_TAP");
  struct ifreq request;
  char sysfs_path[sizeof("/sys/class/net/") + IFNAMSIZ + sizeof("/tun_flags")];
  int control_fd;
  size_t name_len;

  if( !selected || !*selected )
  {
    fprintf(stderr, "PicoTCP-NG: set ELUA_PICO_TAP to an existing Linux TAP interface\n");
    return NULL;
  }
  name_len = strnlen(selected, IFNAMSIZ);
  if( name_len == 0 || name_len >= IFNAMSIZ ||
      strspn(selected, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != name_len )
  {
    fprintf(stderr, "PicoTCP-NG: ELUA_PICO_TAP must be a 1-15 character Linux interface name\n");
    return NULL;
  }
  if( if_nametoindex(selected) == 0 )
  {
    fprintf(stderr, "PicoTCP-NG: TAP interface '%s' is unavailable: %s\n",
            selected, strerror(errno));
    return NULL;
  }
  snprintf(sysfs_path, sizeof(sysfs_path), "/sys/class/net/%s/tun_flags", selected);
  if( access(sysfs_path, F_OK) != 0 )
  {
    fprintf(stderr, "PicoTCP-NG: '%s' is not a Linux TAP interface\n", selected);
    return NULL;
  }

  tap_fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if( tap_fd < 0 )
  {
    fprintf(stderr, "PicoTCP-NG: cannot open /dev/net/tun: %s\n", strerror(errno));
    return NULL;
  }
  memset(&request, 0, sizeof(request));
  request.ifr_flags = IFF_TAP | IFF_NO_PI;
  memcpy(request.ifr_name, selected, name_len + 1);
  if( ioctl(tap_fd, TUNSETIFF, &request) < 0 )
  {
    fprintf(stderr, "PicoTCP-NG: cannot attach to TAP interface '%s': %s\n",
            selected, strerror(errno));
    platform_eth_close();
    return NULL;
  }
  memcpy(tap_name, selected, name_len + 1);

  control_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if( control_fd < 0 )
  {
    fprintf(stderr, "PicoTCP-NG: cannot query TAP MAC address: %s\n", strerror(errno));
    platform_eth_close();
    return NULL;
  }
  memset(&request, 0, sizeof(request));
  memcpy(request.ifr_name, selected, name_len + 1);
  if( ioctl(control_fd, SIOCGIFFLAGS, &request) < 0 ||
      !(request.ifr_flags & IFF_UP) )
  {
    fprintf(stderr, "PicoTCP-NG: TAP interface '%s' is down or unavailable\n", selected);
    close(control_fd);
    platform_eth_close();
    return NULL;
  }
  memset(&request, 0, sizeof(request));
  memcpy(request.ifr_name, selected, name_len + 1);
  if( ioctl(control_fd, SIOCGIFHWADDR, &request) < 0 )
  {
    fprintf(stderr, "PicoTCP-NG: cannot query TAP MAC address: %s\n", strerror(errno));
    close(control_fd);
    platform_eth_close();
    return NULL;
  }
  memcpy(device_mac, request.ifr_hwaddr.sa_data, sizeof(device_mac));
  close(control_fd);
  ++device_mac[5];
  return device_mac;
}

void platform_eth_close(void)
{
  if( tap_fd >= 0 )
  {
    close(tap_fd);
    tap_fd = -1;
  }
  tap_name[0] = '\0';
}

static int send_frame(const void *src, size_t size)
{
  ssize_t written;
  if( tap_fd < 0 || size < PICO_SIZE_ETHHDR || size > SIM_ETH_MAX_FRAME )
    return -1;
  do
    written = write(tap_fd, src, size);
  while( written < 0 && errno == EINTR );
  if( written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) )
    return 0;
  if( written < 0 )
  {
    fprintf(stderr, "PicoTCP-NG: TAP write on '%s' failed: %s\n",
            tap_name, strerror(errno));
    return -1;
  }
  if( (size_t)written != size )
  {
    fprintf(stderr, "PicoTCP-NG: short TAP write on '%s' (%ld of %lu bytes)\n",
            tap_name, (long)written, (unsigned long)size);
    return -1;
  }
  return (int)written;
}

void platform_eth_send_packet(const void *src, u32 size)
{
  (void)send_frame(src, size);
}

u32 platform_eth_get_packet_nb(void *buf, u32 maxlen)
{
  uint8_t discard[SIM_ETH_MAX_FRAME + 1];
  uint8_t *dst = buf ? buf : discard;
  size_t capacity = buf ? maxlen : sizeof(discard);
  ssize_t received;

  if( tap_fd < 0 || capacity == 0 )
    return 0;
  do
    received = read(tap_fd, dst, capacity);
  while( received < 0 && errno == EINTR );
  if( received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) )
    return 0;
  if( received < 0 )
  {
    fprintf(stderr, "PicoTCP-NG: TAP read on '%s' failed: %s\n",
            tap_name, strerror(errno));
    return 0;
  }
  if( received == 0 )
    return 0;
  if( received < PICO_SIZE_ETHHDR || received > SIM_ETH_MAX_FRAME ||
      (buf && (u32)received > maxlen) )
  {
    fprintf(stderr, "PicoTCP-NG: discarded malformed or oversized Ethernet frame on '%s'\n",
            tap_name);
    return 0;
  }
  return (u32)received;
}

void platform_eth_force_interrupt(void)
{
}

u32 platform_eth_get_elapsed_time(void)
{
  return 0;
}

int elua_pico_tap_send(struct pico_device *dev, void *buf, int len)
{
  (void)dev;
  return send_frame(buf, (size_t)len);
}

int elua_pico_tap_poll(struct pico_device *dev, int loop_score)
{
  uint8_t frame[SIM_ETH_MAX_FRAME + 1];

  while( loop_score > 0 )
  {
    u32 len = platform_eth_get_packet_nb(frame, sizeof(frame));
    if( len == 0 )
      break;
    --loop_score;
    if( len > dev->mtu + PICO_SIZE_ETHHDR )
      continue;
    pico_stack_recv(dev, frame, len);
  }
  return loop_score;
}

static void tap_destroy(struct pico_device *dev)
{
  (void)dev;
  platform_eth_close();
}

void elua_pico_tap_set_destroy(struct pico_device *dev)
{
  dev->destroy = tap_destroy;
}

#endif
