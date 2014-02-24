

//----------------------------------------------------------------------------
// Susan Paradiso
// CS502
// Project 4 test -- utils
//
//----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

#include "mailbox.h"
#include "p4test.h"
#include "p4utils.h"


//---------------------------------------------------------------------------
// TSLog
// Thread-Safe Log
//
// msg is a format for the vprintf, then "..." are the (optional and variable)
// args for the format.
// 
// This is a thread-safe printf function. It gets th print_lock then does the
// printing, then releases the lock.
//---------------------------------------------------------------------------
void TSLog(char *msg, ...) {
va_list argp;
  pthread_mutex_lock(&print_lock);
  va_start(argp, msg);
  vprintf(msg, argp);
  va_end(argp);
  fflush(stdout);
  pthread_mutex_unlock(&print_lock);
}

//---------------------------------------------------------------------------
// Err
// This prefixes the message with ERROR, prints the message, then exits with
// a failure. 
//---------------------------------------------------------------------------
void Err(char *msg, ...) {
va_list argp;

  printf("\nERROR, ");
  va_start(argp, msg);
  vprintf(msg, argp);
  va_end(argp);
  printf("\n\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

//---------------------------------------------------------------------------
// HarvestArgs
//---------------------------------------------------------------------------
void HarvestArgs(int argc, char **argv, test_info_t *tinfo) {

  if (argc != 2) {
    printf("Requires one input:\n");
    printf("\ttest number\n");
    Err("Incorrect number of args. \n");
  }

  tinfo->test_num = atoi(argv[1]);

  //Sanity check
  if (tinfo->test_num < 0) {
    Err("Arg(s) less than 0. \n");
  }

}

//---------------------------------------------------------------------------
// ErrDecoder
//---------------------------------------------------------------------------
char *ErrDecoder(int err_code, char *err_str) {
    switch (err_code) {
	case MAILBOX_GOOD:     snprintf(err_str, MAX_ERR_SZ, 
					  "MAILBOX_GOOD (%d)",
					  err_code);
			  	break;
	case MAILBOX_FULL:     snprintf(err_str, MAX_ERR_SZ, 
					  "MAILBOX_FULL (%d)",
					  err_code);
			  	break;
	case MAILBOX_EMPTY:    snprintf(err_str, MAX_ERR_SZ,
					  "MAILBOX_EMPTY (%d)",
					  err_code);
			  	break;
	case MAILBOX_STOPPED:  snprintf(err_str, MAX_ERR_SZ, 
					  "MAILBOX_STOPPED (%d)",
					  err_code);
			  	break;
	case MAILBOX_INVALID:  snprintf(err_str, MAX_ERR_SZ, 
					  "MAILBOX_INVALID (%d)",
					  err_code);
			  	break;
	case MSG_TOO_LONG:     snprintf(err_str, MAX_ERR_SZ, 
					  "MSG_TOO_LONG (%d)",
					  err_code);
			  	break;
	case MSG_ARG_ERROR:    snprintf(err_str, MAX_ERR_SZ, 
					  "MSG_ARG_ERROR (%d)",
					  err_code);
			  	break;
	case MAILBOX_ERROR:    snprintf(err_str, MAX_ERR_SZ, 
					  "MAILBOX_ERROR (%d)",
					  err_code);
			  	break;
	default: 	       snprintf(err_str, MAX_ERR_SZ, 
					  "BAD ERROR CODE, %d",
					  err_code);
			  	break;
  }
  return err_str;
}






