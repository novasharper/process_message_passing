/*
 * test4.c
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

static void Test4a( void );
static void Test4b( void );
static void Test4c( void );

/*
 *  Test4
 *
 *  Performs testing of the ManageMailbox() function.
 *
 *  Tests:
 *  	Test4a
 * 			Verifies the count value of the ManageMailbox call.
 *  		- Empty returns 0 count
 *  		- Incremental send and verify counts
 *  		- Filled returns MAX_QUEUE_SIZE count
 *  		- Burst decrement and verify count
 *
 *  	Test4b
 *  		Verifies MAILBOX_STOPPED is reported correctly.
 *  		- NON-BLOCKING send to stopped mailbox expects MAILBOX_STOPPED
 *  		- BLOCKING SEND to stopped mailbox expects MAILBOX_STOPPED
 *  		- BLOCKING READ to stopped mailbox with messages expects SUCCESS
 *  		- NON-BLOCKING READs to stopped mailbox with messages expects SUCCESS
 *  		- NON-BLOCKING READ to stopped and empty mailbox expects MAILBOX_STOPPED
 *  		- BLOCKING READ to stopped and empty mailbox expects MAILBOX_STOPPED
 *
 *     Test4c
 *   		Verifies mailbox restart handling.  Note - this is not a required
 *   		feature and only has been implemented to work under strict constraints.
 *   	    1. All messages must be read out of the mailbox and the mailbox must
 *   	       report MAILBOX_EMPTY prior to the ManageMailbox() call to start
 *   	       the mailbox.
 *   	    2. All counts and semaphores are forced back to init values.  Full=0,
 *   	       Empty=MAX_QUEUE_SIZE, count=0, queue pointer are initialized to
 *   	       empty state, etc.  This may cause a memory leak if messages were
 *   	       actually left in the mailbox.
 *
 *
 *  Program will exit() on any failure.
 */
int main( int argc, char **argv ) {

	Test4a();
	Test4b();
	Test4c();

	return 0;
}

/*
 *  Test4a
 *  	Verifies the return count value of the ManageMailbox call.
 */
static void Test4a( void ) {

	char *msg = "test4a mailbox count";

	TEST_INTRO( "Mailbox Count Testing" );

	// make sure we're empty to start
	drain_queue( SILENT );
	verify_mailbox_count( 0, MBX_NO_STOP );

	send_msg( "msg one", MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	verify_mailbox_count( 1, MBX_NO_STOP );

	send_msg( "msg two", MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	verify_mailbox_count( 2, MBX_NO_STOP );

	send_msg( "msg three", MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	verify_mailbox_count( 3, MBX_NO_STOP );

	send_msg( "msg four", MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	verify_mailbox_count( 4, MBX_NO_STOP );

	send_msg( "msg five", MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	verify_mailbox_count( 5, MBX_NO_STOP );

	// fill the mailbox
	fill_mailbox( msg );
	verify_mailbox_count( MAX_QUEUE_SIZE, MBX_NO_STOP );

	// read a few messages and verify again and stop the mailbox
	burst_recv( 5, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	verify_mailbox_count( MAX_QUEUE_SIZE-5, MBX_NO_STOP );

	TEST_CLOSING();

}

/*
 *  Test4b
 *  	Verifies MAILBOX_STOPPED is reported correctly.
 */
static void Test4b( void ) {

	char recv_buf[MAX_QUEUE_SIZE];
	char *msg = "test4b testing mailbox stop";
	int rc;

	TEST_INTRO( "Stopped Mailbox Test" );

	// make sure we're empty to start
	drain_queue( SILENT );

	// fill the mailbox
	fill_mailbox( msg );

	// verify and stop
	verify_mailbox_count( MAX_QUEUE_SIZE, MBX_STOP );

	// NON-BLOCKING SEND should fail while MAILBOX_STOPPED
	printf("NON-BLOCKING SEND to stopped mailbox -- expecting failure ...\n");
	rc = send_msg( msg, MBX_NON_BLOCKING, NOISY, ON_SUCCESS_EXIT );
	if (rc != MAILBOX_STOPPED) {
		printf("Mailbox NON-BLOCKING SEND did not report %s as expected.  Response was %s\n",
				mailbox_code(MAILBOX_STOPPED),mailbox_code(rc));
		FAILURE_EXIT();
	}

	// BLOCKING SEND should fail while MAILBOX_STOPPED
	printf("BLOCKING SEND to stopped mailbox -- expecting failure ...\n");
	rc = send_msg( msg, MBX_BLOCK, NOISY, ON_SUCCESS_EXIT );
	if (rc != MAILBOX_STOPPED) {
		printf("Mailbox BLOCKING SEND did not report %s as expected.  Response was %s\n",
				mailbox_code(MAILBOX_STOPPED),mailbox_code(rc));
		FAILURE_EXIT();
	}

	// BLOCKING READ should succeed while MAILBOX_STOPPED if mailbox is not empty
	printf("BLOCKING READ to stopped mailbox with messages -- expecting success ...\n");
	rc = recv_msg( recv_buf, MBX_BLOCK, NOISY, ON_FAILURE_EXIT );
	if (rc != 0) {
		printf("Mailbox BLOCKING READ did not report %s as expected.  Response was %s\n",
				mailbox_code(0),mailbox_code(rc));
		FAILURE_EXIT();
	}

	// should be able to read out the messages queued in the mailbox even though stopped
	printf("NON-BLOCKING READs to stopped mailbox with messages -- expecting success ...\n");
	burst_recv( MAX_QUEUE_SIZE-1, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );

	// extra NON-BLOCKING READ should fail with MAILBOX_STOPPED
	printf("Extra NON-BLOCKING READ to stopped/empty mailbox -- expecting failure ...\n");
	rc = recv_msg( recv_buf, MBX_NON_BLOCKING, NOISY, ON_SUCCESS_EXIT );
	if (rc != MAILBOX_STOPPED) {
		printf("Mailbox NON-BLOCKING READ did not report %s as expected.  Response was %s\n",
				mailbox_code(MAILBOX_STOPPED),mailbox_code(rc));
		FAILURE_EXIT();
	}

	// BLOCKING READ should fail with MAILBOX_STOPPED
	printf("Extra BLOCKING READ to stopped/empty mailbox -- expecting failure ...\n");
	rc = recv_msg( recv_buf, MBX_BLOCK, NOISY, ON_SUCCESS_EXIT );
	if (rc != MAILBOX_STOPPED) {
		printf("Mailbox BLOCKING READ did not report %s as expected.  Response was %s\n",
				mailbox_code(MAILBOX_STOPPED),mailbox_code(rc));
		FAILURE_EXIT();
	}

	TEST_CLOSING();

}

/*
 *   Test4c
 *   	Verifies mailbox restart
 */
static void Test4c( void ) {

	char *msg = "mailbox start test";
	int rc, count;

	TEST_INTRO( "Start Mailbox Test" );

	// make sure we're empty to start
	drain_queue( SILENT );
	verify_mailbox_count( 0, MBX_NO_STOP );

	// Start the mailbox
	printf("Starting the mailbox...\n");
	rc = ManageMailbox( MBX_NO_STOP, &count );
	if (rc != 0) {
		printf("ManageMailbox() failed to start the mailbox! Reason=%s\n",mailbox_code(rc));
		FAILURE_EXIT();
	}

	// check that it's working
	send_recv_verify( msg, NOISY );

	TEST_CLOSING();

}
