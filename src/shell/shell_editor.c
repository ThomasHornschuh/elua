// Shell: 'recv' implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "shell.h"
#include "common.h"
#include "type.h"
#include "platform_conf.h"
#include "xmodem.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "kilo.h"

#ifdef BUILD_XMODEM

const char shell_help_edit[] = "[<path>]\n"
  "  [<path>] - file to edit.\n";
const char shell_help_summary_edit[] = "edit a file";

extern char *shell_prog;

void shell_edit( int argc, char **argv )
{
  kilo_main(argc,argv);
}

#else // #ifdef BUILD_XMODEM

const char shell_help_recv[] = "";
const char shell_help_summary_recv[] = "";

void shell_edit( int argc, char **argv )
{
  shellh_not_implemented_handler( argc, argv );
}

#endif // #ifdef BUILD_XMODEM

