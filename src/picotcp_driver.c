#include "platform_conf.h"
#ifdef BUILD_PICOTCP

#pragma message "Compiling picotcp_driver.c"

#include "platform.h"

#include "pico_stack.h"
#include "pico_device.h"

#define MAX_FRAME 1522

#ifdef PICOTCP_BUFFERED

typedef  uint8_t t_eth_frame_buffer[MAX_FRAME];

static t_eth_frame_buffer buffers[PIOCTCP_NUMBUFFERS];
static u32 frame_len[PIOCTCP_NUMBUFFERS];

static int next_free_buffer = 0;
static int next_read_buffrer = 0;
static u64 fifo_overflow;

#define fifo_empty() ( next_free_buffer== next_read_buffrer )
#define fifo_full() ( ((next_free_buffer + 1) % PIOCTCP_NUMBUFFERS) == next_read_buffrer   )
#define fifo_cnt_incr(i) ( i = (i+1) % PIOCTCP_NUMBUFFERS )

static elua_int_c_handler prev_handler;

#include "console.h"
void check_fifo()
{
  if (next_free_buffer < 0 || next_free_buffer > (PIOCTCP_NUMBUFFERS-1) )
    kassert_fail("next_free_buffer out of range ");
  if (next_read_buffrer < 0 || next_read_buffrer > (PIOCTCP_NUMBUFFERS-1) )
    kassert_fail("next_read_buffrer out of range ");  
}


#endif 



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
#ifdef PICOTCP_BUFFERED
     int old_status=platform_cpu_set_interrupt( INT_ETHERNET_RECV,0, PLATFORM_CPU_DISABLE ); 
     check_fifo();
     while ( loop_score > 0 && !fifo_empty() ) { 
         platform_cpu_set_interrupt( INT_ETHERNET_RECV,0, old_status ); 
         pico_stack_recv( dev,buffers[next_read_buffrer],frame_len[next_read_buffrer] );
         platform_cpu_set_interrupt( INT_ETHERNET_RECV,0, PLATFORM_CPU_DISABLE ); 
         fifo_cnt_incr( next_read_buffrer );
         check_fifo();
         loop_score--;
     }  
     platform_cpu_set_interrupt( INT_ETHERNET_RECV,0, old_status );

#else
    while (loop_score > 0) {
        t_eth_frame_buffer buf;
    
        u32 len = platform_eth_get_packet_nb( buf,sizeof(buf));
        if (len == 0) {
            break;
        }        
        dbg_drv("Received %lu bytes, buffer address %lx\n",len,buf);
        pico_stack_recv(dev, buf, len); /* this will copy the frame into the stack */
        loop_score--;
    }
#endif         
        
    /* return (original_loop_score - amount_of_packets_received) */
    return loop_score;
}

#ifdef PICOTCP_BUFFERED
static void driver_eth_rx_int(elua_int_resnum resnum )
{
u32 len;

  for(;;) {
    len = platform_eth_get_packet_nb( buffers[next_free_buffer],MAX_FRAME);
    if ( len > 0) {
        if (fifo_full()) {
          fifo_overflow++;
        } else {
          frame_len[next_free_buffer]=len;
          fifo_cnt_incr( next_free_buffer );
          check_fifo();
        }
    } else
        break;
  } 
  
  //platform_cpu_get_interrupt_flag( INT_ETHERNET_RECV, resnum, 1 );

  if (prev_handler) prev_handler( resnum );
 
}
#endif 

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

#ifdef PICOTCP_BUFFERED
    platform_cpu_set_interrupt( INT_ETHERNET_RECV,0, PLATFORM_CPU_ENABLE );
    prev_handler = elua_int_set_c_handler( INT_ETHERNET_RECV, driver_eth_rx_int );
    dbg("Ethernet interrupt handler enabled\n");
#endif 
    dbg("Device %s init succesfull\n",name);

    /* Return a pointer to the device struct */
    return eth_dev;
}

#endif
