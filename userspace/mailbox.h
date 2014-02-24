/**
* Adapted from CS-502 Project #3, Fall 2006
*	originally submitted by Cliff Lindsay
* Modified for CS-3013, C-term 2012
*
*/

#ifndef __MAILBOX__
#define __MAILBOX__

#include <stdbool.h>
#include <linux/types.h>

#define NO_BLOCK 0
#define BLOCK   1
#define MAX_MSG_SIZE 128

/**
 * Functions for msgs
 * 
 * */
long SendMsg(pid_t dest, void *msg, int len, bool block);
long RcvMsg(pid_t *sender, void *msg, int *len, bool block);

/**
 * functions for maintaining mailboxes
 * 
 * */
long ManageMailbox(bool stop, int *count);

/**
 * error codes pertaining to mailboxes
 * 
 * */
#define MAILBOX_FULL		1001
#define MAILBOX_EMPTY		1002
#define MAILBOX_STOPPED		1003
#define MAILBOX_INVALID		1004
#define MSG_LENGTH_ERROR	1005
#define MSG_ARG_ERROR		1006
#define MAILBOX_ERROR		1007

#endif

