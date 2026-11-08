// Host I/O operations for eLua 'simulator'

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include "term.h"
#include "host.h"
#include "hostif.h"

#define EOF (-1)
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

static struct termios saved_termios;
static int terminal_is_raw;

void hostif_restore_terminal()
{
  if( terminal_is_raw )
  {
    tcsetattr( STDIN_FILENO, TCSAFLUSH, &saved_termios );
    terminal_is_raw = 0;
  }
}

void hostif_init_terminal()
{
  struct termios raw;

  /* Non-interactive test runs feed stdin through a pipe and need no changes. */
  if( !isatty( STDIN_FILENO ) || tcgetattr( STDIN_FILENO, &saved_termios ) != 0 )
    return;

  raw = saved_termios;
  raw.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG );
  raw.c_iflag &= ~( ICRNL | IXON );
  raw.c_cc[ VMIN ] = 1;
  raw.c_cc[ VTIME ] = 0;
  if( tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw ) == 0 )
    terminal_is_raw = 1;
}

void hostif_putc(char c)
{
	host_write(STDOUT_FILENO, &c, 1);
}

void hostif_putstr(const char *c)
{
  int i = 0;
  while( c[ i ] )
    hostif_putc( c[ i++ ] );
}

int hostif_getch()
{
 	unsigned char ch = 0;
	if(host_read(STDIN_FILENO, &ch, 1) != 1) {
		return EOF;
	}
	return (int)ch;
 
}

int hostif_getch_timeout( unsigned timeout_ms )
{
  fd_set fds;
  struct timeval timeout;

  FD_ZERO( &fds );
  FD_SET( STDIN_FILENO, &fds );
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = ( timeout_ms % 1000 ) * 1000;
  if( select( STDIN_FILENO + 1, &fds, NULL, NULL, &timeout ) <= 0 )
    return EOF;
  return hostif_getch();
}

void* hostif_getmem( unsigned size )
{
  void *pmem = host_mmap( 0, size, (PROT_READ|PROT_WRITE), (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0 );
  return pmem == MAP_FAILED ? NULL : pmem;
}

void hostif_exit()
{
	hostif_restore_terminal();
	host_exit( 0 );
}

int hostif_open( const char* name, int flags, unsigned mode )
{
  return host_open( name, flags, ( mode_t )mode );
}

int hostif_read( int fd, void *buf, unsigned count )
{
  return ( int )host_read( fd, buf, ( size_t )count );
}

int hostif_write( int fd, const void *buf, unsigned count )
{
  return ( int )host_write( fd, buf, ( size_t )count );
}

int hostif_close( int fd )
{
  return host_close( fd );
}

long hostif_lseek( int fd, long pos, int whence )
{
  return host_lseek( fd, pos, whence );
}

s64 hostif_gettime()
{
  struct timeval tv;

  host_gettimeofday( &tv );
  return ( s64 )tv.tv_sec * 1000000 + tv.tv_usec;
}
