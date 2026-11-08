#define _GNU_SOURCE

#include "host.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int host_errno = 0;

static long host_syscall_result( long result )
{
  if( result == -1 )
    host_errno = errno;
  return result;
}

ssize_t host_read( int fd, void *buf, size_t count )
{
  return (ssize_t)host_syscall_result( syscall( SYS_read, fd, buf, count ) );
}

ssize_t host_write( int fd, const void *buf, size_t count )
{
  return (ssize_t)host_syscall_result( syscall( SYS_write, fd, buf, count ) );
}

int host_open( const char *name, int flags, mode_t mode )
{
  return (int)host_syscall_result(
      syscall( SYS_openat, AT_FDCWD, name, flags, mode ) );
}

int host_close( int fd )
{
  return (int)host_syscall_result( syscall( SYS_close, fd ) );
}

off_t host_lseek( int fd, off_t pos, int whence )
{
  return (off_t)host_syscall_result( syscall( SYS_lseek, fd, pos, whence ) );
}

void *host_mmap( void *addr, size_t length, int prot, int flags, int fd,
                 off_t offset )
{
  long result = host_syscall_result(
      syscall( SYS_mmap, addr, length, prot, flags, fd, offset ) );
  return result == -1 ? MAP_FAILED : (void *)result;
}

int host_clock_gettime( struct timespec *ts )
{
  return (int)host_syscall_result( syscall( SYS_clock_gettime, CLOCK_MONOTONIC, ts ) );
}

void host_exit( int status )
{
  syscall( SYS_exit, status );
  abort();
}
