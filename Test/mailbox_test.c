/**
 * Pat Long
 * Feb 21, 2014
 * CS 3013
 * Project 4 - Test Program
 * test1 - Test if messages can be sent and recieved
 * test2 - Tests if programs that chose to wait until able to send a message behave properly
 * test3 - Tests sending message to stopped mailbox
 * test4 - Tests getting message from empty mailbox
 * test5 - Crash test
 */

#include "mailbox.h"

#define CHILD_NUM 50

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
		return;
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
			return;
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
		printf("[%d]FAILED: %d %d\n", childPID, failed, CHILD_NUM - num_mesg);
	} else {
		printf("[%d]PASSED\n", childPID);
	}
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
		return;
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

// CRASH TEST
// Cleaned up version of sample 7
// WILL CRASH KERNEL UNLESS PROPERLY HANDLED
void test5(void) {
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
	printf("PASSED\n");
//	printf("Parent dies.\n");
}

int main(int argc, char **argv) {
	if(argc != 2) {
		printf("Usage: ./mailbox_test [test case number]\n");
		return 0;
	}
	int test_num = atoi(argv[1]);
	switch(test_num) {
		case 1:
			test1( );
			break;
		case 2:
			test2( );
			break;
		case 3:
			test3( );
			break;
		case 4:
			test4( );
			break;
		case 5:
			test5( );
			break;
		default:
			printf("Invalid test case. Test case numbers are between 1-5.\n");
			return 0;
	}
	int status;
	int res = waitpid(-1, &status, 0);
	return 0;
}