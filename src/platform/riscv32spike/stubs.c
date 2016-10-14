#include "platform.h"
#include "platform_conf.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"

int platform_can_exists( unsigned id )
{
  return 0;	
}	

u32 platform_can_setup( unsigned id, u32 clock )
{	
  return 0;	
}

int platform_can_send( unsigned id, u32 canid, u8 idtype, u8 len, const u8 *data )
{
	
	return 0;
}

int platform_can_recv( unsigned id, u32 *canid, u8 *idtype, u8 *len, u8 *data )
{
  return PLATFORM_UNDERFLOW	;
}	

int platform_spi_exists( unsigned id )
{
  return 0;	
}	

u32 platform_spi_setup( unsigned id, int mode, u32 clock, unsigned cpol, unsigned cpha, unsigned databits )
{
  return 0;	
}

spi_data_type platform_spi_send_recv( unsigned id, spi_data_type data )
{
  return NULL;	
}

void platform_spi_select( unsigned id, int is_select )
{
	
}



int platform_i2c_exists( unsigned id )
{
  return 0;	
}

u32 platform_i2c_setup( unsigned id, u32 speed )
{
  return 0;	
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


int platform_pwm_exists( unsigned id )
{
  return 0;	
}

u32 platform_pwm_setup( unsigned id, u32 frequency, unsigned duty )
{
	
  return 0;	
}

void platform_pwm_start( unsigned id )
{
  
}


void platform_pwm_stop( unsigned id )
{
  
}

u32 platform_pwm_set_clock( unsigned id, u32 clock )
{
  return 0;	
}


u32 platform_pwm_get_clock( unsigned id )
{
  return 0;	
}



u64 platform_timer_sys_raw_read()
{
  return 0;	
}



void platform_timer_sys_enable_int()
{
	
}



void platform_timer_sys_disable_int()
{
	
}



