/*
 * test1.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502 Cisco
 *      Project 4
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "mailbox.h"
#include "test_utils.h"

static void basic_send_recv_test( void ) {
	TEST_INTRO( "Basic Send/Receive Test");
	send_recv_verify( "Msg1 Hi", NOISY );
	send_recv_verify( "Msg2 A longer hello message", NOISY );
	send_recv_verify( "Msg3 A much much longer hello message sent to my own mailbox", NOISY );
	TEST_CLOSING();
}

static void slow_send_recv_test( void ) {
	TEST_INTRO( "Slow Send/Receive Test");
	paced_send_recv_verify( "slow_send_recv_test message", 10, NOISY, 1 );
	TEST_CLOSING();
}

static void burst_send_recv_test( void ) {
	TEST_INTRO( "Burst Send/Receive Test");
	paced_send_recv_verify( "burst_send_recv_test message", MAX_QUEUE_SIZE, NOISY, 0 );
	TEST_CLOSING();
}

static void overflow_queue_test( void ) {

	int count = MAX_QUEUE_SIZE;
	char * message = "overflow_queue_test message";

	TEST_INTRO( "Queue Overflow Test" );

	burst_send( message, count, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );

	printf("%d messages written to SendMsg; all should have succeeded\n", count);
	printf("This next SendMsg should fail\n");

	send_msg( message, MBX_NON_BLOCKING, NOISY, ON_SUCCESS_EXIT );

	TEST_CLOSING();

	drain_queue( SILENT );
}

static void underflow_queue_test( void ) {

	int count = 5;
	char * message = "underflow_queue_test message";

	TEST_INTRO( "Queue Underflow Test" );

	burst_send( message, count, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );
	burst_recv( count, MBX_NON_BLOCKING, NOISY, ON_FAILURE_EXIT );

	printf("%d messages written to SendMsg; and read from RcvMsg, all should have succeeded\n", count);
	printf("This next RcvMsg should fail\n");

	recv_msg( message, MBX_NON_BLOCKING, NOISY, ON_SUCCESS_EXIT );

	TEST_CLOSING();

	drain_queue( SILENT );
}

#define MSG1   "a legal message"
#define MSG2   ""
#define MSG3   "a very very very very very very really really really really long message that exceeds the limit of one hundred and twenty eight characters"
#define MSG4   NULL


struct data_struct {
	char *msg;
	int len;
};

static void send_msg_arg_test( void ) {

	int i, rc;

	struct data_struct data[] =
	{
		{ MSG1, 0 },				// normal message 0 length
		{ MSG1, -5 },				// normal message negative length
		{ MSG1, MAX_MSG_SIZE+2 },	// normal message too large length
		{ MSG2, 0 },				// blank message 0 length
		{ MSG2, -99 },				// blank message negative length
		{ MSG2, MAX_MSG_SIZE+20 },	// blank message too large length
		{ MSG2, 0 },				// too large message 0 length
		{ MSG2, -1 }, 				// too large message negative length
		{ MSG2, MAX_MSG_SIZE+99 },  // too large message too large length
		{ MSG3, 0 },				// NULL message 0 length
		{ MSG3, -7 },				// NULL message negative length
		{ MSG3, MAX_MSG_SIZE+42 },   // NULL message too large length
		{ MSG4, 0 },				// NULL message 0 length
		{ MSG4, -14 },				// NULL message negative length
		{ MSG4, MAX_MSG_SIZE+11 }   // NULL message too large length
	};

	TEST_INTRO( "SendMsg Arguments Test" );

	for (i=0; i<(sizeof(data)/sizeof(struct data_struct)); i++) {

		rc = SendMsg( getpid(), data[i].msg, data[i].len, MBX_NON_BLOCKING );

		printf("SendMsg() with msg='%s' and length=%d reported %s !!!\n",data[i].msg,data[i].len,mailbox_code(rc));

		switch(rc) {
		case MSG_TOO_LONG:
		case MSG_ARG_ERROR:
		case MAILBOX_ERROR:
			// good error codes
			break;
		case 0:
		case MAILBOX_FULL:
		case MAILBOX_EMPTY:
		case MAILBOX_STOPPED:
		case MAILBOX_INVALID:
		default:
			FAILURE_EXIT();
			break;
		}
	}

	TEST_CLOSING();
}

static void recv_msg_arg_test( void ) {

	int i, rc;
	pid_t pid;
	char msg[MAX_MSG_SIZE];
	int len;

	TEST_INTRO( "RcvMsg Arguments Test" );

	for (i=0; i<3; i++) {
		switch(i) {
		case 0:
			printf("Testing &PID = NULL\n");
			rc = RcvMsg( NULL, msg, &len, MBX_NON_BLOCKING );
			break;
		case 1:
			printf("Testing &MSG = NULL\n");
			rc = RcvMsg( &pid, NULL, &len, MBX_NON_BLOCKING );
			break;
		case 2:
			printf("Testing &length= NULL\n");
			rc = RcvMsg( &pid, msg, NULL, MBX_NON_BLOCKING );
			break;
		}

		switch(rc) {
		case MSG_ARG_ERROR:
		case MAILBOX_ERROR:
			printf("... correctly reported %s\n",mailbox_code(rc));
			// good error codes
			break;
		case 0:
		case MSG_TOO_LONG:
		case MAILBOX_FULL:
		case MAILBOX_EMPTY:
		case MAILBOX_STOPPED:
		case MAILBOX_INVALID:
		default:
			FAILURE_EXIT();
			break;
		}
	}

	TEST_CLOSING();
}

/*
 *  Test1
 *
 *  Performs a series of basic tests, verifying proper return codes and mailbox message content.
 *  Also performs negative testing for the message NULL and invalid length arguments.  All tests
 *  are run from a single process and the test will exit() on any failure.
 *
 *  Tests:
 *     Basic Send / Receive Test
 *     		Sends three messages one at a time of varying lengths and verifies each is received
 *     		and the message content is correct.
 *
 *     Slow Send Test
 *     		Sends a series of 10 messages paced one each second and verifies the messages are
 *     		received and that the message content is correct.
 *
 *     Burst Send / Receive Test
 *        	Sends a burst of MAX_QUEUE_SIZE messages to test the depth of the mailbox. All messages
 *        	are then burst received and message contents verified.
 *
 *     Overflow Queue Test
 *      	The mailbox is filled by burst sending MAX_QUEUE_SIZE messages.  Another message send,
 *      	NON-BLOCKING then is attempted, with a failure of MAILBOX_FULL expected.
 *
 *	   Underflow Queue Test
 *	   		The mailbox is filled by burst sending MAX_QUEUE_SIZE messages, followed by a burst
 *	   		read of MAX_QUEUE_SIZE.  Another NON-BLOCKING message read is attempted, with a failure
 *	   		of MAILBOX_EMPTY expected.
 *
 *     SendMsg Arguments Test
 *     		Negative tests all arguments to the SendMsg routine.  Tests NULL messages, invalid message
 *     		lengths (too long, 0 and negative).
 *
 *     RcvMsg Arguments Test
 *     		Negative tests all arguments to the RcvMsg routine.  Tests handling of NULL pointers passed
 *     		to RcvMsg() for *pid, *user_buf, and *len.
 *
 *  Program will exit() on any failure.
 */
int main( int argc, char **argv ) {

	basic_send_recv_test();

	slow_send_recv_test();

	burst_send_recv_test();

	overflow_queue_test();

	underflow_queue_test();

	send_msg_arg_test();

	recv_msg_arg_test();

	return 0;
}

