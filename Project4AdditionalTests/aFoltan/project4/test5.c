/*
 * test5.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mailbox.h"
#include "test_utils.h"

/*
 *  message
 *
 *  A helper routine to allocate a message buffer and construct
 *  a unique message body.
 *
 *  Inputs:
 *     from - pid of sender process
 *     to   - pid of receiver process
 *     index - unique index value for message
 *
 *  Returns
 *     char * - the allocated and formatted message
 */
char * message( pid_t from, pid_t to, int index ) {

	char * fail_msg = "Unable to allocate message";
	char * msg;

	msg = malloc( MAX_MSG_SIZE );
	if (msg == NULL) {
		msg = fail_msg;
	} else {
		sprintf(msg,"Message %d From:%u To:%u",index,from,to);
	}

	return msg;
}

/*
 *   do_send_recv
 *
 *   The primary processing loop for both processes.  For a specified amount
 *   of time, the process will do the following:
 *
 *   	- sleep randomly up to 0.25 seconds
 *   	- send a message to the peer process NON-BLOCKING
 *   	- sleep randomly up to 0.25 seconds
 *   	- receive a message NON-BLOCKING
 *
 *   If the send queue becomes full, the message will be dropped.
 *   If the receive queue runs empty, the process will try again next loop.
 *
 *   The sender tracks the count of successful sends, the receiver tracks the count
 *   of successful receives.  After each process completes, it reports it's
 *   send/receive count.
 *
 *   Any unexpected mailbox error code will cause the program to exit().
 */
static void do_send_recv( char * id, pid_t to_pid, int seconds ) {

	char recv_buf[MAX_MSG_SIZE];
	char *send_buf;
	int counter = 0;
	int timeout, rc;
	bool bailout = false;
	pid_t from_pid = getpid();
	int send_count = 0;
	int recv_count = 0;
	timeout = time(NULL) + seconds;

	printf("%s sending to %u\n",id, to_pid);

	srand( time(NULL) );

	do {

		usleep( rand()&0x3FFFF );

		send_buf = message(from_pid, to_pid, ++counter);
		rc = send_msg_to_pid( to_pid, send_buf, MBX_NON_BLOCKING, NOISY, NEVER_EXIT );
		switch(rc) {
			case 0:
				send_count++;
				free(send_buf);
				break;
			case MAILBOX_FULL:	// continue
					break;
				default:
					bailout = true;
					break;
		}

		usleep( rand()&0x3FFFF );

		recv_msg( recv_buf, MBX_NON_BLOCKING, NOISY, NEVER_EXIT );
		switch(rc) {
			case 0:
				recv_count++;
			case MAILBOX_EMPTY:	// continue
				break;
			default:
				bailout = true;
				break;
		}

	} while( time(NULL) < timeout && !bailout);

	printf("--[%u] Complete -- %d sent, %d received\n",getpid(),send_count,recv_count);

	if (bailout) {
		printf("ERROR: exit reason %s",mailbox_code(rc));
		exit(-1);
	}

}


/*
 *  test5
 *
 *  A two process test to send messages between two separate mailboxes.
 *
 *  For a specified amount of time, the process will do the following:
 *
 *   	- sleep randomly up to 0.25 seconds
 *   	- send a message to the peer process NON-BLOCKING
 *   	- sleep randomly up to 0.25 seconds
 *   	- receive a message NON-BLOCKING
 *
 *   If the send queue becomes full, the message will be dropped.
 *   If the receive queue runs empty, the process will try again next loop.
 *
 *   The sender tracks the count of successful sends, the receiver tracks the count
 *   of successful receives.  After each process completes, it reports it's
 *   send/receive count.
 *
 *   Any unexpected mailbox error code will cause the program to exit().
 */
int main( int argc, char **argv ) {

	int run_secs = 10;
	pid_t parent_pid, child_pid;

	TEST_INTRO( "Two Process Test" );

	parent_pid = getpid();
	child_pid = fork();

	if (child_pid < 0) {

		printf("FATAL:: fork() failed\n");
		exit(-1);

	} else if (child_pid == 0) {

		// child process
		do_send_recv( "CHILD", parent_pid, run_secs );

	} else {

		printf("\nParent PID=%u Child PID =%u\n\n", parent_pid, child_pid );

		// parent process
		do_send_recv( "PARENT", child_pid, run_secs );

	}

	sleep(2);

	TEST_CLOSING();

	fflush(stdout);

	return 0;
}

