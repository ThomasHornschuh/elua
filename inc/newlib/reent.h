#ifndef ELUA_REENT_COMPAT_H
#define ELUA_REENT_COMPAT_H

#if defined( ELUA_PICOLIBC ) || defined( ELUA_SIMULATOR )
#ifdef ELUA_SIMULATOR
#include <sys/types.h>
typedef ssize_t _ssize_t;
typedef off_t _off_t;
#endif

// eLua's device-manager callbacks carry a Newlib-style reentrancy context.
// Picolibc does not expose that ABI, so the Spike port supplies only the
// errno field that eLua uses. Picolibc's own errno handling remains intact.
struct _reent {
  int _errno;
};

extern struct _reent elua_reent;

#define _REENT (&elua_reent)

#else
#include_next <reent.h>
#endif

#endif
