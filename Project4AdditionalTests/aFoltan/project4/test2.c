/*
 * test2.c
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

#define NUM_MESSAGES  10

/*
 *  test2a_thread
 *
 *  The main entry point for the receiving thread of Test2a
 *
 *  Inputs:
 *     arg - timeout duration of the thread in seconds
 *
 *  Returns:
 *     the count of messages received
 */
static void * test2a_thread( void * arg ) {

	char recv_buf[MAX_MSG_SIZE];
	int rc;
	int timeout = (int)arg;

	int recv_count = 0;

	time_t quittime = time(NULL);
	quittime += timeout;

	sleep(1);

	do {
		sleep(1);
		rc = recv_msg( recv_buf, MBX_NON_BLOCKING, NOISY, NEVER_EXIT );
		if (rc == 0) {
			recv_count++;
		}
	} while ( time(NULL) < quittime );

	return (void*)recv_count;
}

/**
 *  Test2a
 *
 *  Sending thread for test2a
 */
static void Test2a( void ) {

	pthread_t t1;
	char * msg = "Test2a Send Non-Blocking / Receive Non-blocking";
	int i, rc;
	int recv_count = 0;
	int send_count = 0;

	TEST_INTRO( "Paced Send Non-Blocking / Receive Non-blocking" );

	if (pthread_create ( &t1, NULL, &test2a_thread, (void*)NUM_MESSAGES+2 ) != 0) {
		printf("Failed to create test2a_thread t1!\n");
		FAILURE_EXIT();
	}

	for (i=0;i<NUM_MESSAGES;i++) {
		sleep(1);
		rc = send_msg(msg, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT);
		if (rc == 0) {
			send_count++;
		}
	}

	pthread_join( t1, (void*)&recv_count );

	printf("Thread 0 sent %d messages\n",send_count);
	printf("Thread 1 received %d messages\n",recv_count);

	if (recv_count != send_count) {
		FAILURE_EXIT();
	}

	TEST_CLOSING();
}

/*
 *  test2b_thread
 *
 *  Receiving thread of Test2a
 *
 *  Inputs: --
 *
 *  Returns:
 *     the count of messages received
 *
 */
static void * test2b_thread( void * arg ) {

	char recv_buf[MAX_MSG_SIZE];
	int recv_count = 0;

	burst_recv( MAX_QUEUE_SIZE, MBX_BLOCK, NOISY, ON_FAILURE_EXIT );
	recv_count += MAX_QUEUE_SIZE;

	usleep(1000);

	recv_msg( recv_buf, MBX_BLOCK, NOISY, ON_FAILURE_EXIT );
	recv_count += 1;

	return (void*)recv_count;
}

/*
 *    Test2b
 *
 *    Sending thread of test2b
 */
static void Test2b( void ) {

	pthread_t t1;
	char * msg = "Test2b Burst Send Non-Blocking / Receive Blocking";
	int recv_count = 0;
	int send_count = 0;

	TEST_INTRO( "Send Blocking Test" );

	if (pthread_create ( &t1, NULL, &test2b_thread, NULL ) != 0) {
		printf("Failed to create test2b_thread t1!\n");
		FAILURE_EXIT();
	}

	usleep(100000);

	burst_send( msg, MAX_QUEUE_SIZE, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	send_count += MAX_QUEUE_SIZE;

	usleep(10000);

	send_msg( msg, MBX_BLOCK, NOISY, ON_FAILURE_EXIT );
	send_count += 1;

	pthread_join( t1, (void*)&recv_count );

	printf("Thread 0 sent %d messages\n",send_count);
	printf("Thread 1 received %d messages\n",recv_count);

	if (recv_count != send_count) {
		FAILURE_EXIT();
	}

	TEST_CLOSING();

}


/*
 *   test2c_thread
 *
 *  Receiving thread of Test2c
 *
 *  Inputs: --
 *
 *  Returns:
 *     the count of messages received
 *
 */
static void * test2c_thread( void * arg ) {

	int recv_count = 0;

	burst_recv( 2*MAX_QUEUE_SIZE, MBX_BLOCK, NOISY, ON_FAILURE_EXIT );
	recv_count = 2*MAX_QUEUE_SIZE;

	return (void*)recv_count;
}

/*
 *    Test2c
 *
 *    Sending thread of test2c
 */
static void Test2c( void ) {

	pthread_t t1;
	char * msg = "Test2c Burst Send Blocking / Receive Blocking";
	int recv_count = 0;
	int send_count = 0;

	TEST_INTRO( "Send Blocking Test" );

	if (pthread_create ( &t1, NULL, &test2c_thread, NULL ) != 0) {
		printf("Failed to create test2c_thread t1!\n");
		FAILURE_EXIT();
	}

	burst_send( msg, 2*MAX_QUEUE_SIZE, MBX_BLOCK, NOISY, ON_FAILURE_EXIT );
	send_count = 2*MAX_QUEUE_SIZE;

	pthread_join( t1, (void*)&recv_count );

	printf("Thread 0 sent %d messages\n",send_count);
	printf("Thread 1 received %d messages\n",recv_count);

	if (recv_count != send_count) {
		FAILURE_EXIT();
	}

	TEST_CLOSING();

}

/*
 *  Test2
 *
 *  Performs a series of two-thread tests as described below.
 *
 *  All tests will exit() on any failure.
 *
 *  Tests:
 *     Test2a
 *      	Two thread test with thread T1 sending to thread T2.  T1 sends NUM_MESSAGES
 *  		paced one each second.  NUM_MESSAGES is less than MAX_QUEUE_SIZE.  At the end
 *  		of the test, the number of sent messages is	verified against the number of
 *  		received messages.  Both sending and receiving are NON-BLOCKING.
 *
 *     Test2b
 *      	Two thread test with thread T1 sending to thread T2.  T1 sends MAX_QUEUE_SIZE
 *  		as a burst.  At the end of the test, the number of sent messages is
 *  		verified against the number of received messages.  Sending is performed
 *  		NON-BLOCKING, receive is BLOCKING.
 *
 *     Test2c
 *      	Two thread test with thread T1 sending to thread T2.  T1 sends 2*MAX_QUEUE_SIZE
 *  		as a burst.  At the end of the test, the number of sent messages is
 *  		verified against the number of received messages.  Both sending and receiving
 *  		are BLOCKING.
 *
 */
int main( int argc, char **argv ) {

	TEST_INTRO("Two Thread Testing");
	Test2a();
	Test2b();
	Test2c();
	TEST_CLOSING();

	return 0;
}
