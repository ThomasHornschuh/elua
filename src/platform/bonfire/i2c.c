
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
#ifdef I2C_SDA_BIT
  if (id==0)
    return 1;
  else
#endif
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




u32 platform_i2c_setup( unsigned id, u32 speed )
{
  if (id!=0) return 0;
#ifdef I2C_SDA_BIT

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
  set_SCL(TRUE);
  set_SDA(TRUE);

  return SYSCLK / _i2c_ticks; // return "rounded" speed
#endif
}

// Waits for one clock period
static void platform_i2c_delay(u32 ticks)
{
volatile u64 cnt;

  cnt=platform_timer_sys_raw_read()+ticks;
  while ( platform_timer_sys_raw_read()<=cnt );
}


void platform_i2c_send_start( unsigned id )
{
  if (id!=0) return;
#ifdef I2C_SDA_BIT
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
  set_SCL(FALSE);
#endif
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


void platform_i2c_send_stop( unsigned id )
{

  if (id!=0) return;
#ifdef I2C_SDA_BIT
  set_SCL(TRUE);

  wait_SCL_high();

  platform_i2c_delay(_i2c_hticks); // tSU:STO
  set_SDA(TRUE);

#endif

}


int platform_i2c_send_address( unsigned id, u16 address, int direction )
{
  return 0;
}



int platform_i2c_send_byte( unsigned id, u8 data )
{
int bit;
BOOL v;

  // assume SCL is low on start
  for(bit=7;bit>=0;bit--) {
     v = (data >> bit) & 0x1; // Bit to shift out
     set_SDA(v);
     platform_i2c_delay(_i2c_lticks);
     if (!wait_SCL_high()) return 0;
     platform_i2c_delay(_i2c_hticks);
     set_SCL(FALSE);

  }
  // Handle ack bit
  platform_i2c_delay(_i2c_lticks);
  if (!wait_SCL_high()) return 0;
  platform_i2c_delay(_i2c_hticks);
  BOOL ack= ! get_SDA();
  set_SCL(FALSE);

  return ack;
}


int platform_i2c_recv_byte( unsigned id, int ack )
{
  return 0;
}
