/* Alfredo Porras
 * July 12th, 2011
 * CS 3013
 * Project 4 - test program 2
 * Tests the mailbox to see if programs that chose to wait until able to send
 * a message behave properly.
 * The main process produces 50 children (defined in CHILD_NUM) and
 * sends a message to each child so that they know their parent's PID.
 * All 50 children respond (block == true) back to their parent,
 * making it very likely that the parent's mailbox will become full and some
 * of the children will have to wait for the parent to retrieve enough messages.
 */

#include "mailbox.h"
#include <stdio.h>

#define CHILD_NUM 50

int main() {
  int childCounter;
  
  for(childCounter = 0; childCounter < CHILD_NUM; childCounter++) {
    int childPID = fork();
    
    if(childPID == 0){
      pid_t sender;
      void *msg[128];
      int len;
      bool block = true;
      
      RcvMsg(&sender,msg,&len,block);
    
      
      printf("Message: %s\n", (char *)msg);
      char myMesg[] = "I am your child";
      if(SendMsg(sender, myMesg, 16, block)) {
	printf("Child send failed.\n");
      }
      
      return 0;
    }
    else{
      char mesg[] = "I am your father";
      if (SendMsg(childPID, mesg, 17, false)){
	printf("Send failed\n");
      }
    }
  }
  
  int msgCounter;
  for(msgCounter = 0; msgCounter < CHILD_NUM; msgCounter++) {
    pid_t aSender;
    void *reply[128];
    int mLen;
    bool mBlock = true;
    
    RcvMsg(&aSender,reply,&mLen,mBlock);
    printf("Child %d, enqueued # %d Message: %s\n", aSender, msgCounter, (char *)reply);
  }
  return 0;
}
