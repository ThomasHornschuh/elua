
#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"

#include "bonfire_gpio.h"
#include "mem_rw.h"
#include "console.h"


int platform_i2c_exists( unsigned id )
{
#ifdef I2C_SDA_BIT
  if (id==0)
    return 1;
  else
#endif
    return 0;
}


inline void set_sda(int v)
{
uint32_t mask = 1 << I2C_SDA_BIT;
  if (v)  {
    // High level is realized with setting the port
    // to tri state, the pullups will create the high value
    _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_EN,~mask);
  } else {
    // Low is ativly driven
    _mem_and((void*)GPIO_BASE+GPIO_OUTPUT_VAL,~mask);
    _mem_or((void*)GPIO_BASE+GPIO_OUTPUT_EN,mask);
  }
}


u32 platform_i2c_setup( unsigned id, u32 speed )
{
  if (id!=0) return 0;
#ifdef I2C_SDA_BIT



  return 1;
#endif
}


void platform_i2c_send_start( unsigned id )
{


}



void platform_i2c_send_stop( unsigned id )
{

}


int platform_i2c_send_address( unsigned id, u16 address, int direction )
{
  return 0;
}



int platform_i2c_send_byte( unsigned id, u8 data )
{
  return 0;
}


int platform_i2c_recv_byte( unsigned id, int ack )
{
  return 0;
}
