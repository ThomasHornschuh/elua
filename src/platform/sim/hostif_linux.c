// Host I/O operations for eLua 'simulator'

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include "term.h"
#include "host.h"
#include "hostif.h"
#include "platform_conf.h"
#include "common.h"
#ifdef ELUA_PICOTCP_NG
#include "elua_picotcp.h"
#endif

#define EOF (-1)
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

static struct termios saved_termios;
static int terminal_is_raw;

#if VTMR_NUM_TIMERS > 0
#if defined( BUILD_INT_HANDLERS ) && defined( INT_TMR_MATCH )
#error Simulator virtual timer signal handler cannot run timer interrupt handlers
#endif
#if VTMR_FREQ_HZ > 1000000 || 1000000 % VTMR_FREQ_HZ != 0
#error Simulator virtual timer frequency must divide 1000000
#endif

static void hostif_timer_signal( int signo )
{
  ( void )signo;
  cmn_virtual_timer_cb();
}
#endif

int hostif_init_timers()
{
#if VTMR_NUM_TIMERS > 0
  struct sigaction action;
  struct itimerval timer;

  memset( &action, 0, sizeof( action ) );
  action.sa_handler = hostif_timer_signal;
  action.sa_flags = SA_RESTART;
  sigemptyset( &action.sa_mask );
  if( sigaction( SIGALRM, &action, NULL ) != 0 )
    return -1;

  memset( &timer, 0, sizeof( timer ) );
  timer.it_interval.tv_sec = ( 1000000 / VTMR_FREQ_HZ ) / 1000000;
  timer.it_interval.tv_usec = ( 1000000 / VTMR_FREQ_HZ ) % 1000000;
  timer.it_value = timer.it_interval;
  if( setitimer( ITIMER_REAL, &timer, NULL ) != 0 )
    return -1;
#endif
  return 0;
}

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
#ifdef ELUA_PICOTCP_NG
  for( ;; )
  {
    fd_set fds;
    struct timeval timeout;
    int result;
    unsigned char ch = 0;

    FD_ZERO( &fds );
    FD_SET( STDIN_FILENO, &fds );
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    result = select( STDIN_FILENO + 1, &fds, NULL, NULL, &timeout );
    if( result > 0 )
    {
      if( host_read( STDIN_FILENO, &ch, 1 ) == 1 )
        return (int)ch;
      return EOF;
    }
    if( result == 0 )
      elua_pico_tick();
    else if( errno != EINTR )
      return EOF;
    else
      elua_pico_tick();
  }
#else
 	unsigned char ch = 0;
	if(host_read(STDIN_FILENO, &ch, 1) != 1) {
		return EOF;
	}
	return (int)ch;
#endif
}

int hostif_getch_timeout( unsigned timeout_ms )
{
  fd_set fds;
  struct timeval timeout;
  int result;

  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = ( timeout_ms % 1000 ) * 1000;
  do
  {
    FD_ZERO( &fds );
    FD_SET( STDIN_FILENO, &fds );
    result = select( STDIN_FILENO + 1, &fds, NULL, NULL, &timeout );
  } while( result < 0 && errno == EINTR );
  if( result <= 0 )
    return EOF;
  return hostif_getch();
}

void* hostif_getmem( unsigned size )
{
  void *pmem = host_mmap( 0, size, (PROT_READ|PROT_WRITE), (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0 );
  return pmem == MAP_FAILED ? NULL : pmem;
}

void hostif_exit(int status)
{
#if VTMR_NUM_TIMERS > 0
  sigset_t blocked;
  struct itimerval timer;

  sigemptyset( &blocked );
  sigaddset( &blocked, SIGALRM );
  sigprocmask( SIG_BLOCK, &blocked, NULL );
  memset( &timer, 0, sizeof( timer ) );
  setitimer( ITIMER_REAL, &timer, NULL );
#endif
	hostif_restore_terminal();
	host_exit( status );
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
  struct timespec ts;

  if( host_clock_gettime( &ts ) != 0 )
    return 0;
  return ( s64 )ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
