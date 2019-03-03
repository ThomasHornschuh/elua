#include "platform_conf.h"
#ifdef BUILD_PICOTCP

#pragma message "Compiling picotcp_driver.c"

#include "platform.h"

#include "pico_stack.h"
#include "pico_device.h"

#define MAX_FRAME 1522

#define dbg_drv(...)

static int driver_eth_send(struct pico_device *dev, void *buf, int len)
{
    dbg_drv("Send Packet %d bytes\n",len);
    platform_eth_send_packet(buf, len);
    /* send function must return amount of bytes put on the network - no negative values! */
    return len;
}

static int driver_eth_poll(struct pico_device *dev, int loop_score)
{
    uint8_t buf[MAX_FRAME];
    uint32_t len = 0;

    while (loop_score > 0) {


        len = platform_eth_get_packet_nb( buf,sizeof(buf));
        if (len == 0) {
            break;
        }
        dbg_drv("Received %lu bytes, buffer address %lx\n",len,buf);
        pico_stack_recv(dev, buf, len); /* this will copy the frame into the stack */
        loop_score--;
    }

    /* return (original_loop_score - amount_of_packets_received) */
    return loop_score;
}


struct pico_device *pico_eth_create(const char *name)
{
    /* Create device struct */
    struct pico_device* eth_dev = PICO_ZALLOC(sizeof(struct pico_device));
    dbg_drv("Device Address %lx\n",eth_dev);
    if(!eth_dev) {
        return NULL;
    }

    dbg("Initalize driver\n");
    /* Initialize hardware */
    const uint8_t *mac= platform_eth_init();

    /* Attach function pointers */
    eth_dev->send = driver_eth_send;
    eth_dev->poll = driver_eth_poll;

    /* Register the device in picoTCP */
    if( 0 != pico_device_init(eth_dev, name, mac)) {
        dbg("Device %s init failed.\n",name);
        PICO_FREE(eth_dev);
        return NULL;
    }
    dbg("Device %s init succesfull\n",name);

    /* Return a pointer to the device struct */
    return eth_dev;
}

#endif
