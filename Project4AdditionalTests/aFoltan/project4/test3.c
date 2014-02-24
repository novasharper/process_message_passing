/*
 * test3.c
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

#include "mailbox.h"
#include "test_utils.h"

#define NUM_THREADS                 10
#define NUM_MESSAGES                50000
#define TEST_TIMEOUT_SEC            6

/*
 *  thread_run
 *
 *  The main entry point for the threads
 *
 *  Inputs:
 *     arg - count of seconds the
 */
static void * thread_run( void * arg ) {

	char recv_buf[MAX_QUEUE_SIZE];
	int rc;
	int timeout = (int)arg;

	unsigned int count;
	unsigned int recv_count;

	time_t quittime = time(NULL);
	time_t updatetime;

	quittime += timeout;

	sleep(1);

	recv_count = 0;

	do {

		count = 0;
		updatetime = time(NULL) + 1;

		do {
			rc = recv_msg( recv_buf, MBX_NON_BLOCKING, SILENT, NEVER_EXIT );
			switch(rc) {
			case 0: count++; break;
			case MAILBOX_EMPTY: break;
			case MSG_TOO_LONG:
			case MSG_ARG_ERROR:
			case MAILBOX_ERROR:
			case MAILBOX_FULL:
			case MAILBOX_STOPPED:
			case MAILBOX_INVALID:
			default:
				printf("rcv error %s\n",mailbox_code(rc));
				FAILURE_EXIT();
				break;
			}
		} while ( time(NULL) < updatetime );

		printf("[%lu] ... %u messages received\n",pthread_self(),count);
		recv_count += count;

	} while ( time(NULL) < quittime );

	printf("[%lu] Total messages received = %u\n", pthread_self(), recv_count);

	return (void*)recv_count;
}

/*
 *  Test3
 *  	A multi-thread test where the main thread sources messages and
 *  	multiple threads receive.  This program creates NUM_THREADS (10)
 *  	threads to receive messages.  Each receiving thread will run for
 *  	TEST_TIMEOUT_SEC (6 seconds) in a NON-BLOCKING mode.
 *  	The main thread will run in BLOCKING mode and send a total of
 *  	NUM_MESSAGES messages (50,000).
 *
 *  Program will exit() on any failure.
 */
int main( int argc, char **argv ) {

	pthread_t t[NUM_THREADS];
	char * msg = "Threading test";
	int i, rc;
	int send_count = 0;
	int recv_count = 0;
	int count = 0;

	TEST_INTRO( "Single Send / Multiple Receive Thread Full-speed" );

	for (i=0;i<NUM_THREADS;i++) {
		if (pthread_create ( &t[i], NULL, &thread_run, (void*)TEST_TIMEOUT_SEC ) != 0) {
			printf("Failed to create thread t[%d]!\n",i);
			FAILURE_EXIT();
		}
	}

	sleep(2);

	for (i=0;i<NUM_MESSAGES;i++) {
		rc = send_msg( msg, MBX_BLOCK, SILENT, NEVER_EXIT );
		if (rc == 0) {
			send_count++;
		}
	}

	for (i=0; i<NUM_THREADS; i++) {
		pthread_join( t[i], (void*)&count );
		recv_count += count;
	}

	if (recv_count != send_count) {
		printf("Test3 Failed: recv count %d does not match send count %d!\n",recv_count,send_count);
		FAILURE_EXIT();
	}

	printf("Sent %d messages\n",send_count);
	printf("Received %d messages\n",recv_count);

	TEST_CLOSING();

	return 0;
}
