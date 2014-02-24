/* Alfredo Porras
 * July 12th, 2011
 * CS 3013
 * Project 4 - test program 7
 * Tests if mailbox can be deleted while other processes
 * are holding a pointer to it.
 * Multiple threads are present in the parent process this time.
 * The mailbox is not deleted until the last thread exits.
 * Each thread sleeps for one more second than the previous one.
 */

#include "mailbox.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define CHILD_NUM 50
#define THREAD_NUM 5

void *goToSleep(void *length) {
  printf("This thread is going to sleep.\n");
  usleep((int)length); // 5 seconds
  printf("This thread just awakened.\n");
  pthread_exit(NULL);
}

int main() {
  int childCounter;
  
  // spawn a few threads
  int threadCounter;
  for (threadCounter = 0; threadCounter < THREAD_NUM; threadCounter++) {
    pthread_t newThread;
    int sleepFor = (threadCounter + 1) * 1000000;
    pthread_create(&newThread, NULL, goToSleep, (void *) sleepFor);
  }
  
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
  
  // the parent sleeps for a little time
  // waiting for some of it's children to get a pointer
  // to its mailbox
  // before trying to kill its own process.
  usleep(100000);
  
  pthread_exit(NULL);
}