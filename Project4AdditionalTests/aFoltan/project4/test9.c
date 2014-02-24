/*
 * test9.c
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

#include "mailbox.h"
#include "test_utils.h"

#define  NUM_CHILDREN  50

/*
 *  test9
 *
 *   A multi-process echo test for exit deadlock.
 *
 *   This test forks 50 child processes, sending a message to each as
 *   they block on receive.  The child then echoes the message back to the parent
 *   which ignores it letting the parent mailbox fill up.  Since the number
 *   of children is greater than the max queue size, some children will block
 *   on send.  The parent process then exits from main to test whether the
 *   blocked children will clean up and exit properly.
 *
 *   Any unexpected mailbox error code will cause the program to exit().
 */
int main( int argc, char **argv ) {

	pid_t parent_pid, child_pid;
	int i, rc;
	char msg[MAX_MSG_SIZE];

	TEST_INTRO( "Two Process Test" );

	parent_pid = getpid();

	for (i=0; i<NUM_CHILDREN; i++) {

		child_pid = fork();

		if (child_pid < 0) {

			printf("FATAL:: fork() failed\n");
			exit(-1);

		} else if (child_pid == 0) {	// child process

			char recv_buf[MAX_MSG_SIZE];
			int rc, recv_len;
			pid_t from_pid;

			child_pid = getpid();

			rc = RcvMsg( &from_pid, recv_buf, &recv_len, true );
			if (rc != 0) {
				printf("Child[%d] failed to receive message from parent!\n",child_pid);
				exit(-1);
			}

			rc = SendMsg( from_pid, recv_buf, recv_len, true );
			if (rc == MAILBOX_STOPPED) {
				printf("Child[%d] reports MAILBOX_STOPPED (expected)!\n", child_pid);
			} else {
				printf("Child[%d] returned %s from SendMsg()!\n",child_pid, mailbox_code(rc));
				exit(-1);
			}

			// child done
			exit(0);

		}

		snprintf( msg, MAX_MSG_SIZE, "Message from parent [%d] to child [%d]",parent_pid,child_pid );
		rc = SendMsg( child_pid, msg, strlen(msg), true );
		if (rc != 0) {
			printf("Parent failed to send message to child[%d]!\n", child_pid);
			exit(-1);
		}

	}

	// wait a little while to let the kiddies block on sending to parent
	usleep(500000);

	printf("This test passes if all the children exit cleanly!!\n");

	fflush(stdout);

	return 0;
}

