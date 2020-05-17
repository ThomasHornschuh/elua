#include "platform_conf.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
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

#ifndef ELUA_TFTP_PATHPREFIX
#define ELUA_TFTP_PATHPREFIX "/mmc/tftp/"
#endif

#ifndef ELUA_TFTP_BUFSIZE
#define ELUA_TFTP_BUFSIZE 65535
#endif

typedef  struct {
  FILE * f;
  char filebuf[ELUA_TFTP_BUFSIZE];
} t_file_ctx;

#define tftplog printf //temporary 

// int (*user_cb)(struct pico_tftp_session *session, uint16_t event, uint8_t *block, int32_t len, void *arg)

static int cb_tftp_tx(struct pico_tftp_session *session, uint16_t event,
                        uint8_t *block, int32_t len, void *arg)
{
  tftplog("tx callback\n");
 
  return 0;
}                        


static int cb_tftp_rx(struct pico_tftp_session *session, uint16_t event,
                        uint8_t *block, int32_t len, void *arg)
{
t_file_ctx *ctx = (t_file_ctx *)arg;  
const char *wr_error = "tftp write error, aborting transfer\n";
int written;

  switch (event) {

    case PICO_TFTP_EV_OK:
      if (len>0) {
        written = fwrite(block,1,len,ctx->f);
        if (written!=len) {
          tftplog(wr_error);
          pico_tftp_abort(session,-1,wr_error);
          fclose(ctx->f);
          free(ctx);
        }
      }
      if (len!=512) { // Last block of file...
        fclose(ctx->f);
        free(ctx);
      }
      break;
    case PICO_TFTP_EV_ERR_PEER:
    case PICO_TFTP_EV_ERR_LOCAL:
      tftplog("tftp error %s\n",(char*)block);
      fclose(ctx->f);
      free(ctx);
      break;
    default:
      tftplog("unsupported event: %d\n",event);  
  }
  
  return 0;
}                        

static void tftp_server_callback( union pico_address *addr, uint16_t port,
                                  uint16_t opcode, char *filename, int32_t len )
{

struct pico_tftp_session *session;
char path[128];
t_file_ctx *ctx;

  tftplog("Listem from remote port %d\n",short_be(port));
  
  switch(opcode) {
    case PICO_TFTP_RRQ:
        tftplog("tftp get for %s\n",filename);
        session = pico_tftp_session_setup(addr,PICO_PROTO_IPV4);
        pico_tftp_start_tx(session,port,filename,cb_tftp_tx,NULL);
        break;
    case PICO_TFTP_WRQ:
      tftplog("tftp put for %s\n",filename);
     
      strncpy(path,ELUA_TFTP_PATHPREFIX,sizeof(path));
      strncat(path,filename,sizeof(path)-1);
      tftplog("Trying to create file %s\n",path);
      ctx=malloc(sizeof(t_file_ctx));
      ctx->f=fopen(path,"w");
      setvbuf(ctx->f,ctx->filebuf,_IOFBF,ELUA_TFTP_BUFSIZE); 
      if (!ctx->f) {
        if (errno != ENOENT) {
            perror("tftp: Unable to open file");
          
        }
        pico_tftp_reject_request(addr,port,-1,"file could not be created");
        free(ctx);
      } else {
        session = pico_tftp_session_setup(addr,PICO_PROTO_IPV4);
        pico_tftp_start_rx(session,port,filename,cb_tftp_rx,ctx);
      }  
      break;
  }
}

static void tftp_server_main()
{

   int err=pico_tftp_listen(PICO_PROTO_IPV4,tftp_server_callback);
   tftplog(err?"tfp start failed\n":"tftp start sucessfull\n");
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
