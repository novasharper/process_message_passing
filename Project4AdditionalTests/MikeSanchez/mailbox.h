//Michael Sanchez
//CSC 502 - Cisco
//mailbox.h

#ifndef __MAILBOX__
#define __MAILBOX__

#include <stdbool.h>
#include <unistd.h>
#include <linux/types.h>

#define NO_BLOCK 0
#define BLOCK   1
#define MAX_MSG_SIZE 128

/**
 * Functions for msgs
 * 
 * */
int SendMsg(pid_t dest, void *msg, int len, bool block);

int RcvMsg(pid_t *sender, void *msg, int *len, bool block);

/**
 * functions for maintaining mailboxes
 * 
 * */
int ManageMailbox(bool stop, int *count);

#define MAILBOX_FULL	1001
#define MAILBOX_EMPTY	1002
#define MAILBOX_STOPPED	1003
#define MAILBOX_INVALID	1004
#define MSG_TOO_LONG	1005
#define MSG_ARG_ERROR	1006
#define MAILBOX_ERROR	1007

#endif

