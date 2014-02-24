//---------------------------------------------------------------------------
// Susan Paradiso
// C502 -- Project 4
// mailbox.h for user testing
//---------------------------------------------------------------------------

#ifndef __SUSAN_MAILBOX_DEFINE__
#define __SUSAN_MAILBOX_DEFINE__

#include <stdbool.h>
#include <unistd.h>
#include <linux/types.h>

//---------------------------------------------------------------------------
// defines
//---------------------------------------------------------------------------
#define NO_BLOCK           0
#define BLOCK              1
#define MAX_MSG_SIZE       128
#define NUM_MSGS_MBOX_FULL 32 /*mbox full after this*/


#define MAILBOX_GOOD	0
#define MAILBOX_FULL	1001
#define MAILBOX_EMPTY	1002
#define MAILBOX_STOPPED	1003
#define MAILBOX_INVALID	1004
#define MSG_TOO_LONG	1005
#define MSG_ARG_ERROR	1006
#define MAILBOX_ERROR	1007

#define ERR_NO_CHECK	(0-2)
#define ERR_BAD_LENPTR	(0-3)
#define ERR_BAD_MSGBUF	(0-4)
#define ERR_BAD_DESTPTR	(0-5)



//---------------------------------------------------------------------------
// prototypes
//---------------------------------------------------------------------------
int SendMsg(pid_t dest, void *msg, int len, bool block);
int RcvMsg(pid_t *sender, void *msg, int *len, bool block);
int ManageMailbox(bool stop, int *count);


#endif

