/*
 * mailbox.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#define __NR_mailbox_send	349
#define __NR_mailbox_rcv	350
#define __NR_mailbox_manage	351

/*
 *  SendMsg
 *
 *  Libarary call for sending a message to a process mailbox.
 *
 *  Inputs:
 *    pid - the process ID to which the message is being sent
 *
 *    msg - a pointer to the buffer that contains the message.
 *          A message must be <= MAX_MSG_SIZE bytes.
 *
 *    block - a flag indicating whether the call should block
 *          until the message is able to be sent in the case
 *          when the mailbox is full.
 *
 *  Returns:
 *    0 on success, -1 on failure;  On failure, errno contains
 *          an error code indicating the reason.
 *
 *          See mailbox.h for error code details.
 */
int SendMsg(pid_t to_pid, void *msg, int len, bool block) {
	long rc;
	rc = syscall(__NR_mailbox_send, to_pid, msg, len, block);
	if (rc != 0) {
		return errno;
	}
	return 0;
}

/*
 *  RcvMsg
 *
 *  Libarary call for retrieving a message from a process mailbox.
 *
 *  Inputs:
 *    from_pid - the address of the pid_t that will return
 *          the process ID of the sender process.
 *
 *    msg - the address of the buffer in which the message will
 *          be returned.  This buffer must be >= MAX_MSG_SIZE bytes.
 *
 *    block - a flag indicating whether the call should block
 *          until the a message arrives in the case when the mailbox
 *          is empty.
 *
 *  Returns:
 *    0 on success, -1 on failure;  On failure, errno contains
 *          an error code indicating the reason.
 *
 *          See mailbox.h for error code details.
 */
int RcvMsg(pid_t *from_pid, void *msg, int *len, bool block) {
	long rc;
	rc = syscall(__NR_mailbox_rcv, from_pid, msg, len, block);
	if (rc != 0) {
		return errno;
	}
	return 0;
}

/*
 *  ManageMailbox
 *
 *  Libarary call for managing a mailbox.  This call cane be used to
 *  start / stop a mailbox, and also to retrieve the count of messages
 *  currently help in the mailbox queue.  It operates only on the
 *  calling processes mailbox.
 *
 *  Inputs:
 *    stop - a flag used to control the state of the mailbox.  A mailbox
 *          can be stopped to prevent messages from being sent to it.
 *          Set to true to stop the mailbox, false to restart the mailbox.
 *          The call is idempotent, so a stopped mailbox can be stopped
 *          again without ill effect.
 *
 *    count - the address of the integer in which the current count of
 *          mailbox messages will be returned.
 *
 *  Returns:
 *    0 on success, -1 on failure;  On failure, errno contains
 *          an error code indicating the reason.
 *
 *          See mailbox.h for error code details.
 */
int ManageMailbox(bool stop, int *count) {
	long rc;
	rc = syscall(__NR_mailbox_manage, stop, count);
	if (rc != 0) {
		return errno;
	}
	return 0;
}
