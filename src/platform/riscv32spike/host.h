// host.h -- Defines syscall wrappers to access host OS.

#ifndef _HOST_H
#define _HOST_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>

extern int host_errno;

ssize_t host_read(int file, void* ptr, size_t len);
ssize_t host_write(int file, const void* ptr, size_t len);
int host_open(const char* name, int flags, int mode);
int host_close(int file);
int host_lseek(int file, long ptr, int dir);

#define PROT_READ 0x1   /* Page can be read.  */
#define PROT_WRITE  0x2   /* Page can be written.  */
#define PROT_EXEC 0x4   /* Page can be executed.  */
#define PROT_NONE 0x0   /* Page can not be accessed.  */

#define MAP_SHARED  0x01    /* Share changes.  */
#define MAP_PRIVATE 0x02    /* Changes are private.  */
#define MAP_FIXED 0x10    /* Interpret addr exactly.  */
#define MAP_ANONYMOUS  0x20    /* Don't use a file.  */

// Flags for "open"
#define O_RDONLY	     00
#define O_WRONLY	     01

#define MAP_FAILED (void *)(-1)

void* host_sbrk(ptrdiff_t incr);

int host_gettimeofday(struct timeval* tp, void* tzp);
void host_exit(int status);

#endif // _HOST_H

