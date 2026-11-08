// Included by platform_conf.h for platform customizations

#ifndef __PLATFORM_GENERIC_H__
#define __PLATFORM_GENERIC_H__

#define PLATFORM_HAS_SYSTIMER

// The board generator guards tmr on hardware timers, which sim does not have.
#if defined( MODULE_TMR_LINE ) && VTMR_NUM_TIMERS > 0
#undef MODULE_TMR_LINE
#define MODULE_TMR_LINE _ROM( AUXLIB_TMR, luaopen_tmr, tmr_map )
#endif

#endif // #ifndef __PLATFORM_GENERIC_H__
