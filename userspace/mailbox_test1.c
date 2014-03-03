/**
 * Pat Long
 * Feb 21, 2014
 * CS 3013
 * Project 4 - Test Program
 * test1 - Test if messages can be sent and recieved
 * test2 - Tests if programs that chose to wait until able to send a message behave properly
 * test3 - Tests sending message to stopped mailbox
 * test4 - Tests getting message from empty mailbox
 */

#include "mailbox.h"
#include <stdio.h>

void test1(void) {
	int childPID = fork();

	if(childPID == 0) {
		// Variables to hold message data
		pid_t sender;
		void *msg[MAX_MSG_SIZE];
		int len;
		// Try to get message
		int status_c = RcvMsg(&sender, msg, &len, true);
		if(status_c) { // If status is non-zero, there was an error
			printf("FAILED\n");
		} else {
			printf("PASSED\n");
		}
	} else {
		char mesg[] = "I am your father";
		int status_p = SendMsg(childPID, mesg, 17, false);
		if (status_p) { // If status is non-zero, there was an error
			printf("FAILED\n");
		}
	}
}

void test2(void) {
	int childCounter;
	
	for(childCounter = 0; childCounter < CHILD_NUM; childCounter++) {
		int childPID = fork();
		
		if(childPID == 0){
			pid_t sender;
			void *msg[MAX_MSG_SIZE];
			int len;
			
			RcvMsg(&sender, msg, &len, true);

			char myMesg[] = "I am your child";
			int error = SendMsg(sender, myMesg, 16, true);
		}
		else{
			char mesg[] = "I am your father";
			SendMsg(childPID, mesg, 17, false);
		}
	}
	
	int failed = 0;
	int msgCounter;
	for(msgCounter = 0; msgCounter < CHILD_NUM; msgCounter++) {
		pid_t aSender;
		void *reply[MAX_MSG_SIZE];
		int mLen;
		
		if(RcvMsg(&aSender, reply, &mLen, false)) failed++;
	}
	if(failed) printf("FAILED");
	else printf("PASSED");
}

void test3(void) {
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
		if(error == MAILBOX_STOPPED) {
			printf("PASSED\n");
		} else {
			printf("FAILED\n");
		}
	} else {
		int num_mesg;
		char mesg[] = "I am your father";
		int status_p = SendMsg(childPID, mesg, 17, false);
		ManageMailbox(true, &num_mesg);
	}
}

void test4(void) {
	// Variables to hold message data
	pid_t sender;
	void *msg[MAX_MSG_SIZE];
	int len;
	
	int error = RcvMsg(&sender, msg, &len, false);

	if(error == MAILBOX_EMPTY) {
		printf("PASSED\n");
	} else {
		printf("FAILED\n");
	}
}

int main(void) {
	test1( );
	int status;
	int res = waitpid(-1, &status, 0);
}