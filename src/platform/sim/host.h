// host.h -- Defines POSIX wrappers to access the Linux host OS.

#ifndef _HOST_H
#define _HOST_H

#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

extern int host_errno;

ssize_t host_read( int fd, void *buf, size_t count );
ssize_t host_write( int fd, const void *buf, size_t count );
int host_open( const char *name, int flags, mode_t mode );
int host_close( int fd );
off_t host_lseek( int fd, off_t pos, int whence );
void *host_mmap( void *addr, size_t length, int prot, int flags, int fd,
                 off_t offset );
int host_clock_gettime( struct timespec *ts );
void host_exit( int status );

#endif // _HOST_H
