

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


//---------------------------------------------------------------------------
// SendMsg
//---------------------------------------------------------------------------
int SendMsg(pid_t dest, void *msg, int len, bool block) {
  return syscall(__NR_MAILBOX_SEND, dest, msg, len, block);
}

//---------------------------------------------------------------------------
// RcvMsg
//---------------------------------------------------------------------------
int RcvMsg(pid_t *sender, void *msg, int *len, bool block) {
  return syscall(__NR_MAILBOX_RCV, sender, msg, len, block);
} 

//---------------------------------------------------------------------------
// ManageMailbox
//---------------------------------------------------------------------------
int ManageMailbox(bool stop, int *count) {
  return syscall(__NR_MAILBOX_MANAGE, stop, count);
} 






