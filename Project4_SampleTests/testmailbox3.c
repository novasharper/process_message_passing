/* Alfredo Porras
 * July 12th, 2011
 * CS 3013
 * Project 4 - test program 3
 * Test the behavior od stopped mailboxes
 */

#include "mailbox.h"
#include <stdio.h>
#include <unistd.h>

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
  
  usleep(3000000);
  printf("Parent awoke from sleep.\n");
  ManageMailbox(true, &childCounter);
  printf("Mailbox stopped.\n");
  
  // retrieve all enqueued messages
  while( childCounter > 0) {
    pid_t aSender;
    void *reply[128];
    int mLen;
    RcvMsg(&aSender, reply, &mLen, true);
    printf("Child %d, dequeueing # %d Message: %s\n", aSender, childCounter, (char *)reply);
    childCounter--;
  }
  
  // attemp to retrieve from and empty and stopped mailbox
  pid_t aSender2;
  void *reply2[128];
  int mLen2;
  RcvMsg(&aSender2, reply2, &mLen2, true);
  
  // atempt to send a message to a stopped mailbox
  int childPID2 = fork();
  if(childPID2 == 0){
    pid_t sender3;
    void *msg3[128];
    int len3;
    bool block = true;
    RcvMsg(&sender3,msg3,&len3,block);
    printf("Message: %s\n", (char *)msg3);
    char myMesg2[] = "I am your child";
    if(SendMsg(sender3, myMesg2, 16, block)) {
      printf("Child send failed.\n");
    }
  }
  else{
    char mesg3[] = "I am your father";
    if (SendMsg(childPID2, mesg3, 17, false)){
      printf("Send failed\n");
    }
  }
  
  return 0;
}
