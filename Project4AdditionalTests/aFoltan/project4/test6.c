/*
 * test6.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mailbox.h"
#include "test_utils.h"
#include "pidlist.h"

#define DEBUG  false

struct stats_data {
	int sent;
	int send_full;
	int rcvd;
	int recv_empty;
};

/*
 *  do_mailbox_test
 *
 *  The primary processing loop of each process testing the mailbox.
 *  Processes randomly choose another process to send a message to,
 *  then attempt to receive a message from their own mailbox.  Both
 *  sending and receiving are NON-BLOCKING.
 *
 *  Statistics are collect to show:
 *  	- number of messages successfully sent
 *  	- number of sends to a full mailbox (MAILBOX_FULL)
 *  	- number of messages received
 *  	- number of reads of an empty mailbox (MAILBOX_EMPTY)
 */
void do_mailbox_test( int timeout, struct stats_data * stats ) {

	char msg[MAX_MSG_SIZE];
	int rc;
	int recv_len;
	pid_t sender_pid;
	pid_t dest_pid;

	time_t quittime = time(NULL);
	quittime += timeout;

	sleep(1);

	do {

		dest_pid = pidlist_get_random();
		sprintf( msg, "From: %u, To: %u", getpid(), dest_pid );
		rc = SendMsg( dest_pid, msg, strlen(msg)+1, NO_BLOCK );
		switch(rc) {
		case 0:		// success
			stats->sent++;
			if (DEBUG) printf("%u sent %s\n",getpid(),msg);
			break;
		case MAILBOX_FULL:
			stats->send_full++;
			if (DEBUG) printf("%u unable to send to %u due to MAILBOX_FULL\n",getpid(),dest_pid);
			break;
		case MAILBOX_INVALID:
			printf("SendMsg [%u] %s\n",dest_pid,mailbox_code(rc));
			break;
		default:
			printf("SendMsg error [%d] sending to %u\n",rc,dest_pid);
			exit(-1);
		}

		if (DEBUG) usleep( UP_TO_250_MSEC );

		rc = RcvMsg( &sender_pid, msg, &recv_len, NO_BLOCK );
		switch(rc) {
		case 0:		// success
			stats->rcvd++;
			if (DEBUG) printf("%u rcvd %s\n",getpid(),msg);
			break;
		case MAILBOX_EMPTY:
			stats->recv_empty++;
			if (DEBUG) printf("%u MAILBOX_EMPTY\n",getpid());
			break;
		case MAILBOX_STOPPED:
			printf("%u unable receive due to MAILBOX_STOPPED\n",getpid());
			break;
		default:
			printf("do_mailbox_test RECV error %d\n",rc);
			exit(-1);
		}

		if (DEBUG) usleep( UP_TO_250_MSEC );

	} while ( time(NULL) < quittime );

	printf("PROC %u done.\n",getpid());
}

/*
 *  new_process
 *
 *  The routine for forking a new process.  The child starts the test,
 *  the parent adds the child PID to the PIDLIST.  The child process
 *  will run for the number of seconds specified by 'timeout'.
 *
 */
void new_process( int timeout ) {

	pid_t pid;

	pid = fork();

	if (pid < 0) {
		printf("FATAL:: fork() failed\n");
		exit(-1);
	} else if (pid == 0) {			// child process

		struct stats_data stats;
		stats.rcvd = stats.recv_empty = stats.sent = stats.send_full = 0;

		// set new seed for random number generation
		SEED();

		do_mailbox_test( timeout, &stats );

		pidlist_remove_pid( getpid() );

		printf("Sent  : %d\n",stats.sent);
		printf("Rcvd  : %d\n",stats.rcvd);
		printf("Full  : %d\n",stats.send_full);
		printf("Empty : %d\n",stats.recv_empty);

		exit(0);

	} else {						// parent process
		// add the new pid into the mailbox storm
		if (pidlist_add_pid( pid ) == -1) {
			printf("Failed to add PID %u to the global PID list -- %d processes already exist\n",
					pid, pidlist_get_count());
		}
	}

}


/*
 *  Test6
 *
 *  A multi-process messaging test.  This test spawns 100 processes which
 *  utilize a shared memory PIDLIST library to track the process IDs of the
 *  active processes.  Every process has access to the PIDLIST, and therefore
 *  can send to every other process.  The test runs for 10 seconds and
 *  processes randomly choose another process to send a message to,
 *  then attempt to receive a message from their own mailbox.  Both
 *  sending and receiving are NON-BLOCKING.
 *
 *  Each process collects statistics about:
 *  	- number of messages successfully sent
 *  	- number of sends to a full mailbox (MAILBOX_FULL)
 *  	- number of messages received
 *  	- number of reads of an empty mailbox (MAILBOX_EMPTY)
 *
 *   Any unexpected mailbox error code will cause the program to exit().
 */
int main( int argc, char **argv ) {

	int i;

	// parse command line
	int max_processes = 100;
	int timeout = 10;

	TEST_INTRO( "Multi-Process / Multi-Threaded Testing" );

	if (pidlist_init() != 0) {
		printf("Failed to initialize the PID list manager!\n");
		exit(-1);
	}

	printf("pidlist_init() success\n");

	for(i=0;i<max_processes;i++) {
		new_process( timeout );
	}

	printf("%d processes are running for the next %d seconds sending and receiving messages randomly ...\n",
			max_processes, timeout);

	while( (wait(NULL)>0) || (errno != ECHILD) );

	TEST_CLOSING();

	fflush(stdout);

	return 0;
}

