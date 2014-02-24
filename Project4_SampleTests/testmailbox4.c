/* Alfredo Porras
 * July 12th, 2011
 * CS 3013
 * Project 4 - test program 4
 * Test message flushing
 */

#include "mailbox.h"
#include <stdio.h>

int main() {
  int childPID = fork();
  
  if(childPID == 0){
    usleep(3000000);
    printf("Child awakes.\n");
    return 0;
  }
  else{
    char mesg[] = "I am your father";
    if (SendMsg(childPID, mesg, 17, false)){
      printf("Send failed\n");
    }
    printf("Message sent.\n");
    char mesg2[] = "I am your father";
    if (SendMsg(childPID, mesg2, 17, false)){
      printf("Send failed\n");
    }
    printf("Message sent. Wait 5s\n");
    usleep(5000000);
    printf("Parent awakes.\n");
    return 0;
  }
}