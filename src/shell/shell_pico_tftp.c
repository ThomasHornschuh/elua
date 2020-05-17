#include "platform_conf.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "shell.h"
#include "common.h"
#include "type.h"

#if defined(BUILD_PICOTCP) && defined(BUILD_TFTP)

#include "pico_stack.h"
#include "pico_tftp.h"

#pragma message "Build TFTP server"

const char shell_help_edit[] = "[<path>]\n"
  "  [<path>] - file to edit.\n";
const char shell_help_summary_edit[] = "edit a file";

extern char *shell_prog;


// int (*user_cb)(struct pico_tftp_session *session, uint16_t event, uint8_t *block, int32_t len, void *arg)

static int cb_tftp_tx(struct pico_tftp_session *session, uint16_t event,
                        uint8_t *block, int32_t len, void *arg)
{
  printf("tx callback\n");
  return 0;
}                        


static int cb_tftp_rx(struct pico_tftp_session *session, uint16_t event,
                        uint8_t *block, int32_t len, void *arg)
{
  printf("rx callback event: %d len: %ld \n",event,len);
  return 0;
}                        

static void tftp_server_callback( union pico_address *addr, uint16_t port,
                                  uint16_t opcode, char *filename, int32_t len )
{

struct pico_tftp_session *session;

  printf("Listem from remote port %d\n",short_be(port));
  session = pico_tftp_session_setup(addr,PICO_PROTO_IPV4);
  switch(opcode) {
    case PICO_TFTP_RRQ:
      printf("tftp get for %s\n",filename);
      pico_tftp_start_tx(session,port,filename,cb_tftp_tx,NULL);
      break;
    case PICO_TFTP_WRQ:
      printf("tftp put for %s\n",filename);
      pico_tftp_start_rx(session,port,filename,cb_tftp_rx,NULL);
      break;
  }
}

static void tftp_server_main()
{

   int err=pico_tftp_listen(PICO_PROTO_IPV4,tftp_server_callback);
   printf(err?"tfp start failed\n":"tftp start sucessfull\n");
}





void shell_tftp( int argc, char **argv )
{
  tftp_server_main();
}

#else 

const char shell_help_edit[] = "";
const char shell_help_summary_edit[] = "";

void shell_tftp( int argc, char **argv )
{
  shellh_not_implemented_handler( argc, argv );
}


#endif
