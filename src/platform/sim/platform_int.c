#include "platform_conf.h"

#if defined( BUILD_INT_HANDLERS )

#include "platform.h"
#include "elua_int.h"

static int sim_tmr_set_status( elua_int_resnum resnum, int status )
{
  ( void )resnum;
  ( void )status;
  return PLATFORM_INT_NOT_HANDLED;
}

static int sim_tmr_get_status( elua_int_resnum resnum )
{
  ( void )resnum;
  return PLATFORM_INT_NOT_HANDLED;
}

static int sim_tmr_get_flag( elua_int_resnum resnum, int clear )
{
  ( void )resnum;
  ( void )clear;
  return PLATFORM_INT_NOT_HANDLED;
}

void platform_int_init( void )
{
}

const elua_int_descriptor elua_int_table[ INT_ELUA_LAST ] =
{
  { sim_tmr_set_status, sim_tmr_get_status, sim_tmr_get_flag }
};

#endif
