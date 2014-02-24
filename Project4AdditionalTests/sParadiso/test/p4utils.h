
//---------------------------------------------------------------------------
// Susan Paradiso
// CS502
// Project 4 -- utils.h
//
//---------------------------------------------------------------------------

#ifndef __SUSAN_P4UTIL_DEFINE__
#define __SUSAN_P4UTIL_DEFINE__

#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>

#include "p4test.h"

#define __NR_MAILBOX_SEND       341
#define __NR_MAILBOX_RCV        342
#define __NR_MAILBOX_MANAGE     343


//---------------------------------------------------------------------------
// prototypes
//---------------------------------------------------------------------------

void TSLog(char *msg, ...);
void Err(char *msg, ...);
void HarvestArgs(int argc, char **argv, test_info_t *tinfo);
char *ErrDecoder(int err_code, char *err_str);


int SendMsg(pid_t dest, void *msg, int len, bool block);
int RcvMsg(pid_t *sender, void *msg, int *len, bool block);
int ManageMailbox(bool stop, int *count);


#endif

