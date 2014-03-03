/* Alfredo Porras
 * July 12th, 2011
 * CS 3013
 * Project 4 - test program 1
 * Tests if messages can be sent and received.
 */

#include "mailbox.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  int childPID = fork();
  
  if(childPID == 0){
    pid_t sender;
    void *msg[128];
    int len;
    bool block = true;
    int status = RcvMsg(&sender,msg,&len,block);
    printf("Message received. %d\n",status);
    printf("Message: %s\n", (char *) msg);
    exit(0);
  }
  else{
    char mesg[] = "I am your father";
    printf("Sending Message to child.\n");
    int a = SendMsg(childPID, mesg, 17, false);
    if (a){
      printf("Send failed, %d\n", a);
    }
  }
  return 0;
}