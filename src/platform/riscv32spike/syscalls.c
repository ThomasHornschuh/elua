//========================================================================
// syscalls.c : Newlib operating system interface                       
//========================================================================
// This is the maven implementation of the narrow newlib operating
// system interface. It is based on the minimum stubs in the newlib
// documentation, the error stubs in libnosys, and the previous scale
// implementation. Please do not include any additional system calls or
// other functions in this file. Additional header and source files
// should be in the machine subdirectory.
//
// Here is a list of the functions which make up the operating system
// interface. The file management instructions execute syscall assembly
// instructions so that a proxy kernel (or the simulator) can marshal up
// the request to the host machine. The process management functions are
// mainly just stubs since for now maven only supports a single process.
//
//  - File management functions
//     + open   : (v) open file
//     + lseek  : (v) set position in file
//     + read   : (v) read from file
//     + write  : (v) write to file
//     + fstat  : (z) status of an open file
//     + stat   : (z) status of a file by name
//     + close  : (z) close a file
//     + link   : (z) rename a file
//     + unlink : (z) remote file's directory entry
//
//  - Process management functions
//     + execve : (z) transfer control to new proc
//     + fork   : (z) create a new process 
//     + getpid : (v) get process id 
//     + kill   : (z) send signal to child process
//     + wait   : (z) wait for a child process
//     
//  - Misc functions
//     + isatty : (v) query whether output stream is a terminal
//     + times  : (z) timing information for current process
//     + sbrk   : (v) increase program data space
//     + _exit  : (-) exit program without cleaning up files
//
// There are two types of system calls. Those which return a value when
// everything is okay (marked with (v) in above list) and those which
// return a zero when everything is okay (marked with (z) in above
// list). On an error (ie. when the error flag is 1) the return value is
// always an errno which should correspond to the numbers in
// newlib/libc/include/sys/errno.h 
//
// Note that really I think we are supposed to define versions of these
// functions with an underscore prefix (eg. _open). This is what some of
// the newlib documentation says, and all the newlib code calls the
// underscore version. This is because technically I don't think we are
// supposed to pollute the namespace with these function names. If you
// define MISSING_SYSCALL_NAMES in xcc/src/newlib/configure.host
// then xcc/src/newlib/libc/include/_syslist.h will essentially define
// all of the underscore versions to be equal to the non-underscore
// versions. I tried not defining MISSING_SYSCALL_NAMES, and newlib
// compiled fine but libstdc++ complained about not being able to fine
// write, read, etc. So for now we do not use underscores (and we do
// define MISSING_SYSCALL_NAMES).
//
// See the newlib documentation for more information 
// http://sourceware.org/newlib/libc.html#Syscalls

#include <machine/syscall.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include "host.h"

//------------------------------------------------------------------------
// environment                                                          
//------------------------------------------------------------------------
// A pointer to a list of environment variables and their values. For a
// minimal environment, this empty list is adequate. We used to define
// environ here but it is already defined in
// xcc/src/newlib/libc/stdlib/environ.c so to avoid multiple definition
// errors we have commented this out for now.
//
// char* __env[1] = { 0 };
// char** environ = __env;
              
//------------------------------------------------------------------------
// open                                                                 
//------------------------------------------------------------------------
// Open a file.

#define syscall_errno(n, a, b, c, d) \
        __internal_syscall(n, (long)(a), (long)(b), (long)(c), (long)(d))

//long __syscall_error(long a0)
//{
  //errno = -a0;
  //return -1;
//}

int host_open(const char* name, int flags, int mode)
{
  return syscall_errno(SYS_open, name, flags, mode, 0);
}


//------------------------------------------------------------------------
// lseek                                                                
//------------------------------------------------------------------------
// Set position in a file.

int host_lseek(int file, long ptr, int dir)
{
  return syscall_errno(SYS_lseek, file, ptr, dir, 0);
}

//----------------------------------------------------------------------
// read                                                                 
//----------------------------------------------------------------------
// Read from a file.


ssize_t host_read(int file, void* ptr, size_t len)
{
  return syscall_errno(SYS_read, file, ptr, len, 0);
}

//------------------------------------------------------------------------
// write                                                                
//------------------------------------------------------------------------
// Write to a file.

ssize_t host_write(int file, const void* ptr, size_t len)
{
  return syscall_errno(SYS_write, file, ptr, len, 0);
}


int host_gettimeofday(struct timeval* tp, void* tzp)
{
  return syscall_errno(SYS_gettimeofday, tp, 0, 0, 0);
}

//------------------------------------------------------------------------
// close                                                                
//------------------------------------------------------------------------
// Close a file.

int host_close(int file) 
{
  return syscall_errno(SYS_close, file, 0, 0, 0);
}


//----------------------------------------------------------------------
// sbrk                                                                 
//----------------------------------------------------------------------
// Increase program data space. As malloc and related functions depend
// on this, it is useful to have a working implementation. The following
// is suggested by the newlib docs and suffices for a standalone
// system.

void* host_sbrk(ptrdiff_t incr)
{
  static unsigned long heap_end;

  if (heap_end == 0) {
    long brk = syscall_errno(SYS_brk, 0, 0, 0, 0);
    if(brk == -1)
	    return (void*)-1;
    heap_end = brk;
  }

  if (syscall_errno(SYS_brk, heap_end + incr, 0, 0, 0) != heap_end + incr)
    return (void*)-1;

  heap_end += incr;
  return (void*)(heap_end - incr);
}

//------------------------------------------------------------------------
// _exit                                                                
//------------------------------------------------------------------------
// Exit a program without cleaning up files.

void host_exit(int exit_status)
{
  syscall_errno(SYS_exit, exit_status, 0, 0, 0);
  while (1);
}
