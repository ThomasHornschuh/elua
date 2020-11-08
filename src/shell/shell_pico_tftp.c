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
#include "pico_ipv4.h"
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
  uint16_t opcode;
  char filebuf[ELUA_TFTP_BUFSIZE];
} t_file_ctx;

#define tftplog printf //temporary 

// int (*user_cb)(struct pico_tftp_session *session, uint16_t event, uint8_t *block, int32_t len, void *arg)

static int cb_tftp_txrx(struct pico_tftp_session *session, uint16_t event,
                        uint8_t *block, int32_t _len, void *arg)
{
t_file_ctx *ctx = (t_file_ctx *)arg;  
const char *trans_error = "tftp  error, aborting transfer\n"; 
uint8_t buffer[PICO_TFTP_PAYLOAD_SIZE];
int len;


switch (event) {

    case PICO_TFTP_EV_OK:
     
      if ( ctx->opcode==PICO_TFTP_RRQ ) {
         // send
        len = fread(buffer,1,PICO_TFTP_PAYLOAD_SIZE,ctx->f);
        if ( len<0 ) {
          tftplog(trans_error);
          pico_tftp_abort(session,-1,trans_error);
          return 0;
        } else {
          pico_tftp_send(session,buffer,len);
        }

      } else {

         //recevice
         if ( _len>0 ) {
            int written = fwrite(block,1,_len,ctx->f);
            if (written!=_len) {
              tftplog(trans_error);
              pico_tftp_abort(session,-1,trans_error);
              return 0;
            }
        }
      }
      break;
    case PICO_TFTP_EV_ERR_PEER:
    case PICO_TFTP_EV_ERR_LOCAL:
      tftplog("tftp error %s at session %lx\n",(char*)block, (uint32_t)session);
      break;
    case  PICO_TFTP_EV_SESSION_CLOSE:
      tftplog("tftp  session %lx closed\n",(uint32_t)session);
      fclose(ctx->f);
      free(ctx);
      break;
    default:
      tftplog("unsupported event: %d\n",event);  
  } 
  return 0;
}                        


static t_file_ctx *open_file(char * filename, char* mode, uint16_t opcode)
{
char path[128];
t_file_ctx *ctx;
FILE *f;

  strncpy(path,ELUA_TFTP_PATHPREFIX,sizeof(path));
  strncat(path,filename,sizeof(path)-1);
  tftplog("open file %s mode %s\n",path,mode);
  
  f=fopen(path,mode);
  if (!f) {
    if (errno != ENOENT) {
        perror("tftp: Unable to open file");
    }
    return NULL;
  } else {
    ctx = malloc(sizeof(t_file_ctx));
    if (!ctx) {
      fclose(f);
      return NULL; // TODO: Better error handling
    }
    setvbuf(f,ctx->filebuf,_IOFBF,ELUA_TFTP_BUFSIZE);
    ctx->f = f;
    ctx->opcode = opcode;
    return ctx; 
  }  
}


static void tftp_server_callback( union pico_address *addr, uint16_t port,
                                  uint16_t opcode, char *filename, int32_t len )
{

struct pico_tftp_session *session;
t_file_ctx *ctx;
char ip_s[16];

  pico_ipv4_to_string(ip_s,addr->ip4.addr);
  tftplog("new request from remote address %s port %d opcode %d\n", ip_s,short_be(port),opcode);
  
  switch(opcode) {
    case PICO_TFTP_RRQ:
        ctx = open_file(filename,"r",opcode);
        if (!ctx) { 
          pico_tftp_reject_request(addr,port,-1,"file could not be opened");
          return;
        } else {
          session = pico_tftp_session_setup(addr,PICO_PROTO_IPV4);
          pico_tftp_start_tx(session,port,filename,cb_tftp_txrx,ctx);
        }
        break;
    case PICO_TFTP_WRQ:
        ctx = open_file(filename,"w",opcode);
        if (!ctx) { 
          pico_tftp_reject_request(addr,port,-1,"file could not be created");
          return;
        } else {
          session = pico_tftp_session_setup(addr,PICO_PROTO_IPV4);
          pico_tftp_start_rx(session,port,filename,cb_tftp_txrx,ctx);
        }
        break;
    default:
        return;    
  }
  tftplog("tftp %s session %lx for %s\n",opcode==PICO_TFTP_RRQ?"tx":"rx", (uint32_t)session, filename);  
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
