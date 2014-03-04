/**
 * Pat Long
 * Feb 21, 2014
 * CS 3013
 * Project 4 - Test Program
 * test_send_message - Test if messages can be sent and recieved
 * test_message_overflow_wait - Tests if programs that chose to wait until able to send a message behave properly
 * test_send_stopped_mailbox - Tests sending message to stopped mailbox
 * test_recieve_empty_mailbox - Tests getting message from empty mailbox
 * crash_test - Crash test
 */

#include "mailbox.h"

#define CHILD_NUM 50
#define IGNORE -123124

#define do_test(name) \
switch(name()) {\
	case IGNORE:\
		break;\
	case 0:\
		printf("FAILED - Test %s\n",#name);\
		exit(1);\
	default:\
		printf("PASSED - Test %s\n",#name);\
	}

#define expect_true(val) \
	if (!(val)) {\
		printf("Failed expect_true for %s\n", #val);\
		return 0;\
	}

int re_fork() {
	pid_t child = fork();
	if (child) {
		int status;
		waitpid(child, &status, 0);
		exit(status);
	}
}

int test_send_message(void) {
	int childPID = fork();

	if(childPID == 0) {
		// Variables to hold message data
		pid_t sender;
		void *msg[MAX_MSG_SIZE];
		int len;
		// Try to get message
		int status_c = RcvMsg(&sender, msg, &len, true);
		if(status_c) { // If status is non-zero, there was an error
			return 0;
		} else {
			return 1;
		}
	} else {
		char mesg[] = "I am your father";
		int status_p = SendMsg(childPID, mesg, 17, false);
		if (status_p) { // If status is non-zero, there was an error
			return 0;
		}
	}
}

int test_message_overflow_wait(void) {
	int childCounter;
	int childPID;
	for(childCounter = 0; childCounter < CHILD_NUM; childCounter++) {
		childPID = fork();
		
		if(childPID == 0){
			pid_t sender;
			void *msg[MAX_MSG_SIZE];
			int len;
			
			RcvMsg(&sender, msg, &len, true);

			char myMesg[] = "I am your child";
			int error = SendMsg(sender, myMesg, 16, true);
			return 0;
		}
		else{
			char mesg[] = "I am your father";
			SendMsg(childPID, mesg, 17, true);
		}
	}

	int failed = 0;
	int msgCounter;
	int num_mesg;
	printf("[%d] Managing my mailbox\n", childPID);
	ManageMailbox(false, &num_mesg);
	printf("[%d] Done with my mailbox\n", childPID);
	for(msgCounter = 0; msgCounter < CHILD_NUM; msgCounter++) {
		pid_t aSender;
		char *reply[MAX_MSG_SIZE];
		int mLen;
		
		if(RcvMsg(&aSender, reply, &mLen, true)) {
			failed++;
			printf("Message failed\n");
		} else {
			printf("Message recieved: %s\n", (char*)reply);
		}
	}
	if(failed) {
		return 0;
	} else {
		return 1;
	}
}

int test_send_stopped_mailbox(void) {
	int childPID = fork();

	if(childPID == 0) {
		// Variables to hold message data
		pid_t sender;
		void *msg[MAX_MSG_SIZE];
		int len;
		// Try to get message
		int status_c = RcvMsg(&sender, msg, &len, true);
		usleep(1000);
		int error = SendMsg(sender, msg, len, false);
		return error == MAILBOX_STOPPED;
	} else {
		int num_mesg;
		char mesg[] = "I am your father";
		int status_p = SendMsg(childPID, mesg, 17, false);
		ManageMailbox(true, &num_mesg);
	}
}

int test_recieve_empty_mailbox(void) {
	// Variables to hold message data
	pid_t sender;
	void *msg[MAX_MSG_SIZE];
	int len;
	
	int error = RcvMsg(&sender, msg, &len, false);

	return error == MAILBOX_EMPTY;
}

// CRASH TEST
// Cleaned up version of sample 7
// WILL CRASH KERNEL UNLESS PROPERLY HANDLED
int crash_test(void) {
	int childCounter;
	
	// fork enough children so that they can all send a message
	// to the parent and hold a pointer to it's mailbox
	for(childCounter = 0; childCounter < CHILD_NUM; childCounter++) {
		int childPID = fork();
		
		if(childPID == 0){
			pid_t sender;
			void *msg[128];
			int len;
			bool block = true;
			
			RcvMsg(&sender,msg,&len,block);
		
			
//			printf("Message: %s\n", (char *)msg);
			char myMesg[] = "I am your child";
			int error = SendMsg(sender, myMesg, 16, block);
			if(error) {
//				printf("Child send failed. %d\n", error
			}
			return;
		}
		else{
			char mesg[] = "I am your father";
			if (SendMsg(childPID, mesg, 17, false)){
//				printf("Send failed\n");
			}
		}
	}
	
	// the parent sleeps for a little time
	// waiting for some of it's children to get a pointer
	// to its mailbox
	// before trying to kill its own process.
	usleep(1000);
	int status;
	int res = waitpid(-1, &status, 0);
	return 1;
}

int main(int argc, char **argv) {
	do_test(test_send_message);
	do_test(test_message_overflow_wait);
	re_fork();
	do_test(test_send_stopped_mailbox);
	re_fork();
	do_test(test_recieve_empty_mailbox);
	do_test(crash_test);
	int status;
	int res = waitpid(-1, &status, 0);
	return 0;
}