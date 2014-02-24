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
  
  printf("Sending messages to children and waiting for responses\n");

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
  printf("Mailbox stopped. %d messages present\n", childCounter);
  
  printf("Retrieving enqueued messages\n");
  // retrieve all enqueued messages
  while( childCounter > 0) {
    pid_t aSender;
    void *reply[128];
    int mLen;
    int ret = RcvMsg(&aSender, reply, &mLen, true);
    printf("Ret: %d Child %d, dequeueing # %d Message: %s\n", ret, aSender, childCounter, (char *)reply);
    childCounter--;
  }
  
  printf("Attempting to get message from empty and stopped mailbox\n");
  // attemp to retrieve from an empty and stopped mailbox
  pid_t aSender2;
  void *reply2[128];
  int mLen2;
  RcvMsg(&aSender2, reply2, &mLen2, true);
  
  printf("Attempting to send a message to stopped mailbox\n");
  // atempt to send a message to a stopped mailbox
  int childPID2 = fork();
  if(childPID2 == 0){
    pid_t sender3;
    void *msg3[128];
    int len3;
    bool block = true;
    int ret = RcvMsg(&sender3,msg3,&len3,block);
    printf("Ret: %d Message: %s\n", ret, (char *)msg3);
    char myMesg2[] = "I am your child";
    ret = SendMsg(sender3, myMesg2, 16, block);
    if(ret) {
      printf("Ret %d Child send failed.\n", ret);
    }
  }
  else{
    char mesg3[] = "I am your father";
    int ret = SendMsg(childPID2, mesg3, 17, false);
    if (ret){
      printf("Ret: %d Send failed\n", ret);
    }
  }
  //sleep(5); 
  return 0;
}
