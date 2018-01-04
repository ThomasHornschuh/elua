
#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"

#include "bonfire_gpio.h"
#include "mem_rw.h"
#include "console.h"

#include <math.h>
#include <stdio.h>


#define I2C_TIMEOUT 1000000L // 1 sec

int platform_i2c_exists( unsigned id )
{

  if (id==0)
    return 1;
  else
    return 0;
}


inline void set_bit(int bitnum,BOOL v)
{
uint32_t mask = 1 << bitnum;
  if (v)  {
    // High level is realized with setting the port
    // to tri state, the pullups will create the high value
    _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_EN,~mask);
  } else {
    // Low is activly driven
    _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_VAL,~mask);
    _mem_or((void*)GPIO_BASE+GPIO_OUTPUT_EN,mask);
  }
}

inline BOOL get_bit(int bitnum)
{
uint32_t mask = 1 << bitnum;

  return (_read_word((void*)GPIO_BASE+GPIO_INPUT_VAL) & mask)?TRUE:FALSE;
}


inline void set_SDA(BOOL v) { set_bit(I2C_SDA_BIT,v); }
inline void set_SCL(BOOL v) { set_bit(I2C_SCL_BIT,v); }
inline BOOL get_SDA() { return get_bit(I2C_SDA_BIT); }
inline BOOL get_SCL() { return get_bit(I2C_SCL_BIT); }


// period = round(1.0/speed * 10^6)

inline u32 calc_i2c_ticks(u32 speed_hz)
{
  return SYSCLK/speed_hz;
}

static u32 _i2c_ticks;
static u32 _i2c_hticks;
static u32 _i2c_lticks;


static void platform_i2c_delay(u32 ticks)
{
volatile u64 cnt;

  cnt=platform_timer_sys_raw_read()+ticks;
  while ( platform_timer_sys_raw_read()<=cnt );
}


u32 platform_i2c_setup( unsigned id, u32 speed )
{
  if (id!=0) return 0;


  if (!platform_timer_sys_available()) {
     fprintf(stderr,"Software I2C module requires systimer\n");
     return 0;
  }
  _i2c_ticks = calc_i2c_ticks(speed);
  if (speed <= 100000) {
    _i2c_hticks = _i2c_ticks / 2;
    _i2c_lticks = _i2c_ticks / 2;
  } else {
    _i2c_hticks = _i2c_ticks / 4; // in fast mode
    _i2c_lticks = _i2c_ticks / 2;
  }
  printk("i2c ticks set to: %ld\n",_i2c_ticks);
  set_SCL(FALSE);
  platform_i2c_delay(_i2c_lticks/2);
  set_SDA(TRUE);
  platform_i2c_delay(_i2c_lticks/2);
  set_SCL(TRUE);
  return SYSCLK / _i2c_ticks; // return "rounded" speed

}


static BOOL wait_SCL_high()
{
  set_SCL(TRUE);
  // wait for SCL  really going high (clock stretching...)
  timer_data_type s = platform_timer_read_sys();
  while (!get_SCL()) {
    if ((platform_timer_read_sys()-s) > I2C_TIMEOUT ) {
       fprintf(stderr,"I2C clock stretch timeout exceeded\n");
       return FALSE;
    }
  }
  return TRUE;
}


void platform_i2c_send_start( unsigned id )
{
  set_SDA(TRUE);
  if (!get_SDA()) {
     platform_i2c_send_stop(id);
     if (!get_SDA()) {
       fprintf(stderr,"i2c send_start: SDA line busy error\n");
       return;
     }
  }

  platform_i2c_delay(_i2c_lticks/2);
  set_SCL(TRUE);

  timer_data_type s = platform_timer_read_sys();
  // wait until bus idle
  while (!get_SCL() || !get_SDA() ) {
    if ((platform_timer_read_sys()-s) > I2C_TIMEOUT ) {
       fprintf(stderr,"I2C bus idle timeout exceeded\n");
       return;
    }
  }
  set_SDA(FALSE);
  platform_i2c_delay(_i2c_hticks); //t HD_STA

}



void platform_i2c_send_stop( unsigned id )
{

  platform_i2c_delay(_i2c_lticks/8); // wait a short moment
  // Assume SCL is low,
  set_SDA(FALSE);
  wait_SCL_high();

  platform_i2c_delay(_i2c_hticks); // tSU:STO
  set_SDA(TRUE);

}


int platform_i2c_send_address( unsigned id, u16 address, int direction )
{
uint8_t adr = ((address & 0x7f) << 1) | (direction==PLATFORM_I2C_DIRECTION_RECEIVER?1:0) ;

  return platform_i2c_send_byte(id,adr);
}



int platform_i2c_send_byte( unsigned id, u8 data )
{
int bit;
BOOL v;

  set_SCL(FALSE);

  platform_i2c_delay(_i2c_lticks/2);
  for(bit=7;bit>=0;bit--) {
     v = (data >> bit) & 0x1; // Bit to shift out
     set_SDA(v);
     platform_i2c_delay(_i2c_lticks/2);
     if (!wait_SCL_high()) return 0;
     platform_i2c_delay(_i2c_hticks);
     set_SCL(FALSE);
     platform_i2c_delay(_i2c_lticks/2);

  }
  // Handle ack bit
  set_SDA(TRUE);
  platform_i2c_delay(_i2c_lticks/2);
  if (!wait_SCL_high()) return 0;
  platform_i2c_delay(_i2c_hticks);
  BOOL ack= ! get_SDA();
  set_SCL(FALSE);

  return ack;
}


int platform_i2c_recv_byte( unsigned id, int ack )
{
int bit;
uint8_t byte=0;

  set_SCL(FALSE);

  for(bit=7;bit>=0;bit--) {

     platform_i2c_delay(_i2c_lticks);
     if (!wait_SCL_high()) return -1;
     platform_i2c_delay(_i2c_hticks);
     byte |= get_SDA() << bit ;
     set_SCL(FALSE);

  }
  // Handle ack bit
  platform_i2c_delay(_i2c_lticks/2);
  set_SDA(ack?0:1);
  platform_i2c_delay(_i2c_lticks/2);
  if (!wait_SCL_high()) return -1;
  platform_i2c_delay(_i2c_hticks);
  set_SCL(FALSE);
  platform_i2c_delay(_i2c_lticks/8); // wait a short moment
  set_SDA(TRUE); // release SDA line

  return byte;
}
