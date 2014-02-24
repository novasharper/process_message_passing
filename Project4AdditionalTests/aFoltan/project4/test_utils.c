/*
 * test_utils.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "mailbox.h"
#include "test_utils.h"

/*
 *  mailbox_code
 *
 *  Return a string representation of a mailbox error result
 *  code suitable for printing to the console.
 *
 *  Inputs:
 *    rc - a mailbox result code from SendMsg(), RcvMsg() or
 *         ManageMailbox()
 *
 *  Returns:
 *    char * - a pointer to a NULL terminated string suitable for
 *             printing
 */
char * mailbox_code( int rc ) {
	switch(rc) {
	case 0:                return "MAILBOX_SUCCESS";
	case MAILBOX_FULL:     return "MAILBOX_FULL";
	case MAILBOX_EMPTY:    return "MAILBOX_EMPTY";
	case MAILBOX_STOPPED:  return "MAILBOX_STOPPED";
	case MAILBOX_INVALID:  return "MAILBOX_INVALID";
	case MSG_TOO_LONG:     return "MSG_TOO_LONG";
	case MSG_ARG_ERROR:    return "MSG_ARG_ERROR";
	case MAILBOX_ERROR:    return "MAILBOX_ERROR";
	case MAILBOX_START_ERR_NOT_EMPTY: return "MAILBOX_START_ERR_NOT_EMPTY";
	case MAILBOX_START_ERR_STOP_PENDING: return "MAILBOX_START_ERR_STOP_PENDING";
	default:
		printf("rc=%d ",rc); return "UNKNOWN";
	}
}

/*
 *  test_closing
 *
 *  Format a standard test case introduction message
 *
 *  Inputs:
 *    title - test title
 *    desc - test description
 *
 *  Returns: --
 */
void test_intro( const char * title, char * desc ) {
	printf("\n\n\n");
	printf("------------------- %s %s -------------------\n\n",title, desc);
}

/*
 *  test_closing
 *
 *  Format a standard test case closing message
 *
 *  Inputs:
 *    title - test title
 *
 *  Returns: --
 */
void test_closing( const char * title ) {
	printf( "\n%s Complete -- PASS!\n\n",title);
}


/*
 *  check_expected
 *
 *  Routine to compare the expected result to the actual result; output a message,
 *  flush and exit.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *    recv_buf - the buffer to copy the message into
 *    block    - an indication of whether the RcvMsg() call should block
 *    silent   - an indication of whether this routine should print progress messages
 *               to the console
 *    expected - indicates the expected condition
 *    exit_condition - indicates the exit condition
 *
 *  Returns: --
 */
void check_expected( expected_result_t expected, expected_result_t exit_condition ) {

	if (expected == exit_condition) {
		printf("Test -- FAILED!\n");
		FAILURE_EXIT();
	}
}

/*
 *  recv_msg
 *
 *  A helper method to retrieve a message from the current process mailbox. RcvMsg() is invoked
 *  NON-BLOCKING, and any unexpected response results in a failure.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *    recv_buf - the buffer to copy the message into
 *    block    - an indication of whether the RcvMsg() call should block
 *    silent   - an indication of whether this routine should print progress messages
 *               to the console
 *    expected - indicates how to handle SUCCESS or FAILURE responses;
 *
 *  Returns:
 *    int - the result of the RcvMsg() call
 */
int recv_msg( char *recv_buf, bool block, bool silent, expected_result_t expected ) {

	int recv_len;
	pid_t from_pid;

	int rc;

	rc = RcvMsg( &from_pid, recv_buf, &recv_len, block );
	if (rc == 0) {
		if (!silent) printf("[%u] RcvMsg [From: %u, To: %u] %s >> SUCCESS\n",
				getpid(), from_pid, getpid(), recv_buf);
		check_expected( expected, ON_SUCCESS_EXIT );
	} else {
		if (!silent) printf("[%u] RcvMsg >> FAILED, code=%s\n",
				getpid(), mailbox_code(rc));
		check_expected( expected, ON_FAILURE_EXIT );
	}

	return rc;
}

/*
 *  send_msg_to_pid
 *
 *  A helper method to send a message to the specified process.  SendMsg is invoked
 *  NON-BLOCKING, and any unexpected response results in a failure.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *    pid      - the PID of the destination process
 *    message  - the message to send, receive and verify
 *    block    - an indication of whether the RcvMsg() call should block
 *    silent   - an indication of whether this routine should print progress messages
 *               to the console
 *    expected - indicates how to handle SUCCESS or FAILURE responses;
 *
 *  Returns:
 *    int - the result of the SendMsg() call
 */
int send_msg_to_pid( pid_t to_pid, char * message, bool block, bool silent, expected_result_t expected ) {

	char send_buf[MAX_MSG_SIZE];
	int send_len;
	int rc;

	if (message == NULL) {
		printf("send_msg invoked with msg=NULL!\n");
		FAILURE_EXIT();
	}

	strcpy(send_buf,message);
	send_len = strlen(send_buf)+1;

	rc = SendMsg( to_pid, send_buf, send_len, block );
	if (rc == 0) {
		if (!silent) printf("[%u] SendMsg [From: %u, To: %u] %s >> SUCCESS\n",
				getpid(), getpid(), to_pid, message);
		check_expected( expected, ON_SUCCESS_EXIT );
	} else {
		if (!silent) printf("[%u] SendMsg [From: %u, To: %u] %s >> FAILED, reason=%s\n",
				getpid(), getpid(), to_pid, message, mailbox_code(rc));
		check_expected( expected, ON_FAILURE_EXIT );
	}

	return rc;
}

/*
 *  send_msg
 *
 *  A helper method to send a message to the current process.  SendMsg is performed
 *  NON-BLOCKING, and any unexpected response results in a failure.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *    message  - the message to send, receive and verify
 *    block    - an indication of whether the RcvMsg() call should block
 *    silent   - an indication of whether this routine should print progress messages
 *               to the console
 *    expected - indicates how to handle SUCCESS or FAILURE responses;
 *
 *  Returns:
 *    int - the result of the SendMsg() call
 */
int send_msg( char * message, bool block, bool silent, expected_result_t expected ) {

	char send_buf[MAX_MSG_SIZE];
	int send_len;
	int rc;
	pid_t to_pid = getpid();
	pid_t from_pid = getpid();

	if (message == NULL) {
		// this test routine does not allow sending of a NULL
		printf("send_msg invoked with msg=NULL!\n");
		FAILURE_EXIT();
	}

	// truncation check
	send_len = strlen(message)+1;

	if (send_len > MAX_MSG_SIZE) {
		printf("WARNING:: send_msg: message too long -- truncating!\n");
		send_len = MAX_MSG_SIZE;
	}

	strncpy(send_buf,message,send_len);
	send_buf[MAX_MSG_SIZE-1] = '\0';	// always guarantee NULL terminated

	rc = SendMsg( to_pid, send_buf, send_len, block );
	if (rc == 0) {
		if (!silent) printf("[%u] SendMsg [From: %u, To: %u] %s >> SUCCESS\n",
				getpid(), from_pid, to_pid, message);
		check_expected( expected, ON_SUCCESS_EXIT );
	} else {
		if (!silent) printf("[%u] SendMsg [From: %u, To: %u] %s >> FAILED, reason=%s\n",
				getpid(), from_pid, to_pid, message, mailbox_code(rc));
		check_expected( expected, ON_FAILURE_EXIT );
	}

	return rc;
}

/*
 *  send_recv_verify
 *
 *  A helper method to send a message to the current process, then read that message back,
 *  and verify the contents and length of the received message against the sent message.
 *  Both SendMsg and RcvMsg is performed NON-BLOCKING, and any unexpected response results
 *  in a failure.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *    message - the message to send, receive and verify
 *    silent - controls whether there are printf outputs of the progress
 *  	         during the execution.
 *
 *  Returns: --
 */
void send_recv_verify( char * message, bool silent ) {

	char recv_buf[MAX_MSG_SIZE];

	send_msg( message, MBX_NON_BLOCKING, silent, ON_FAILURE_EXIT );
	recv_msg( recv_buf, MBX_NON_BLOCKING, silent, ON_FAILURE_EXIT );

	if (strcmp(message, recv_buf)!=0) {
		if (!silent) printf("Message verify FAILED -- sent '%s', received '%s'\n",message, recv_buf);
		FAILURE_EXIT();
	} else {
		if (!silent) printf("Message successfully verified!\n");
	}

}

/*
 *  drain_queue
 *
 *  Reads messages from the mailbox until the mailbox responds either MAILBOX_EMPTY,
 *  or MAILBOX_STOPPED.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *  	silent - controls whether there are printf outputs of the progress
 *  	         during the execution.
 *
 *  Returns: --
 */
void drain_queue( bool silent ) {
	char recv_buf[MAX_MSG_SIZE];
	int rc=0, read_count=0;

	do {
		rc = recv_msg( recv_buf, false, silent, NEVER_EXIT );
		if (rc == 0) read_count++;
	} while (rc == 0);

	if ( rc != MAILBOX_EMPTY && rc != MAILBOX_STOPPED) {
		printf("Failed to drain mailbox, return code = %s\n", mailbox_code(rc));
		FAILURE_EXIT();
	}

	if (!silent) printf("Draining all messages from mailbox .... %d messages read\n", read_count);
}

/*
 *  paced_send_recv_verify
 *
 *  Tests a single processes send-receive-verify path by sending 'count' messages
 *  paced by 1 second sleep intervals.  To make each message unique, the base_msg
 *  will be prefixed by the index of the count.
 *
 *  Any failure will result in program exit();
 *
 *  Inputs:
 *  	base_msg - the base message string sent to the destination.  This may
 *  	         be truncated in order to fit the index count and stay within
 *  	         MAX_MSG_SIZE bytes.  It should be readable for printing on
 *  	         the receive side.
 *      count -  the number of messages to send.  Also relates to the number of
 *               seconds this function will take to complete execution.
 *  	silent - controls whether there are printf outputs of the progress
 *  	         during the execution.
 *  	sleep_time - the delay in seconds between the sends/receive/verify
 *
 *  Returns: --
 */
void paced_send_recv_verify( char * base_msg, int count, bool silent, int sleep_time ) {

	int i, len, base_len;
	char msg[MAX_MSG_SIZE+1];

	base_len = strlen(base_msg)+1;

	for (i=0;i<count;i++) {
		len = sprintf(msg, "#%d ", i);
		strncpy( &msg[len], base_msg, (base_len<(MAX_MSG_SIZE-len)?base_len:(MAX_MSG_SIZE-len-1)) );
		msg[MAX_MSG_SIZE-1] = '\0';		// guarantee NULL terminated
		send_recv_verify( msg, silent );
		if (sleep_time) sleep(sleep_time);
	}
}

/*
 * slow_send
 *
 * A helper call to to send 'count' messages paced at a rate of one message per second.
 * If an error response is received, processing will continue (or not) based on the
 * setting of 'expected' (ON_FAILURE_EXIT, ON_SUCCESS_EXIT, NEVER_EXIT).
 *
 * Any failure will result in program exit();
 *
 * Input:
 *   msg - the message to send
 *   count - the number of messages to send
 *   block - an indication of whether the RcvMsg() call should block
 *   silent - an indication of whether this routine should print progress messages
 *            to the console
 *   expected - indicates how to handle SUCCESS or FAILURE responses;
 *
 * Returns: --
 */
void slow_send( char * msg, int count, bool block, bool silent, expected_result_t expected ) {

	int i;
	for (i=0;i<count;i++) {
		send_msg( msg, block, silent, expected );
		sleep(1);
	}
}

/*
 * burst_send
 *
 * A helper call to to send 'burst_count' number of messages to the mailbox.
 * If an error response is received, processing will continue (or not) based on the
 * setting of 'expected' (ON_FAILURE_EXIT, ON_SUCCESS_EXIT, NEVER_EXIT).
 *
 * Any failure will result in program exit();
 *
 * Input:
 *   msg - the message to send
 *   burst_count - the number of messages to send
 *   block - an indication of whether the RcvMsg() call should block
 *   silent - an indication of whether this routine should print progress messages
 *            to the console
 *   expected - indicates how to handle SUCCESS or FAILURE responses;
 *
 * Returns: --
 */
void burst_send( char * msg, int burst_count, bool block, bool silent, expected_result_t expected ) {

	int i;
	for (i=0;i<burst_count;i++) {
		send_msg(msg,block,silent,expected);
	}
}

/*
 * burst_recv
 *
 * A helper call to receive 'burst_count' number of messages from the mailbox.
 * This routine will continue to invoke SendMsg() until burst_count messages have
 * been received.  If an error response is received (ie MAILBOX_EMPTY, MAILBOX_INVALID, etc),
 * processing will respond based on the setting of 'expected' (ON_FAILURE_EXIT,
 * ON_SUCCESS_EXIT, NEVER_EXIT)..
 *
 * Any failure will result in program exit();
 *
 * Input:
 *   burst_count - the number of messages to receive
 *   block - an indication of whether the RcvMsg() call should block
 *   silent - an indication of whether this routine should print progress messages
 *            to the console
 *   expected - indicates how to handle SUCCESS or FAILURE responses;
 *
 * Returns: --
 */
void burst_recv( int burst_count, bool block, bool silent, expected_result_t expected ) {

	char recv_buf[MAX_MSG_SIZE];

	int i;
	for (i=0;i<burst_count;i++) {
		recv_msg(recv_buf,block,silent,expected);
	}
}

/*
 * fill_mailbox
 *
 * A helper call to fill the mailbox with the provided message.  This routine will
 * continue to invoke SendMsg() until the MAILBOX_FULL response is received.
 *
 * Any failure will result in program exit();
 *
 * Input:
 *   msg - the message to send while filling the mailbox
 *
 * Returns: --
 */
void fill_mailbox( char * msg ) {

	int rc, send_count=0;

	do {
		rc = send_msg( msg, MBX_NON_BLOCKING, SILENT, NEVER_EXIT );
		if (rc == 0) send_count++;
	} while ( rc == 0 );

	if (rc != MAILBOX_FULL) {
		printf("Failed to fill mailbox, return code = %s\n", mailbox_code(rc));
		FAILURE_EXIT();
	}

	printf("Filled mailbox to maximum %d message count by adding %d messages\n", MAX_QUEUE_SIZE,send_count);

}

/*
 * stop_mailbox
 *
 * A helper call to stop the mailbox by calling ManageMailbox() with
 * stop flag == true;
 *
 * Any failure will result in program exit();
 *
 * Input: --
 * Returns: --
 */
void stop_mailbox( void ) {

	int rc, count;

	// stop the mailbox
	rc = ManageMailbox( MBX_STOP, &count );

	if (rc != 0) {
		printf("Failed to stop the mailbox! Reason=%s\n",mailbox_code(rc));
		FAILURE_EXIT();
	}
}

/*
 * verify_mailbox_count
 *
 * Reads the current mailbox count and verifies the count agrees with the
 * provided expected value.
 *
 * Any failure will result in program exit();
 *
 * Input:
 *   expected_count - the count of messages expected to be in the mailbox.
 *
 *   stop - a flag to control the state of the mailbox.  true = stopped,
 *          false = restart or continue in active state.
 *
 * Returns: --
 */
void verify_mailbox_count( int expected_count, bool stop ) {

	int rc, count;

	// verify the count
	rc = ManageMailbox( stop, &count );

	if (rc != 0) {
		printf("Failed to stop the mailbox! Reason=%s\n",mailbox_code(rc));
		FAILURE_EXIT();
	}

	if (count != expected_count) {
		printf("Incorrect mailbox count=%d, expected %d\n",count,expected_count);
		exit(-1);
	}

	printf("Mailbox message count %d verfied\n",expected_count);

}

/*
 *  randmax
 *
 *  Helper to generate a pseudorandom number between 0 and max-1
 *
 *  Inputs:
 *    max - the maximum of the range
 *
 *  Returns:
 *    int - random number between 0 and max-1
 */
int randmax( int max ) {
    int val = (int)((double)max*(double)rand()/((double)RAND_MAX+1.0));
    return val;
}
