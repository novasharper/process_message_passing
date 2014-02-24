/**
* Adapted from CS-502 Project #3, Fall 2006
*	originally submitted by Cliff Lindsay
* Modified for CS-502, Summer 2011 by Alfredo Porras
* Adapted for CS-502 at Cisco Systems, Fall 2011
*
*/

#include "mailbox.h"

#define __NR_mailbox_send	341
#define __NR_mailbox_rcv	342
#define __NR_mailbox_manage	343

/**
 * Functions for msgs
 * 
 * */
int SendMsg(pid_t dest, void *msg, int len, bool block) {
  return syscall(__NR_mailbox_send, dest, msg, len, block);
} 	// int SendMsg

int RcvMsg(pid_t *sender, void *msg, int *len, bool block){
  return syscall(__NR_mailbox_rcv, sender, msg, len, block);
}	// int RcvMsg

/**
 * functions for maintaining mailboxes
 * 
 * */
int ManageMailbox(bool stop, int *count){
  return syscall(__NR_mailbox_manage, stop, count);
}	// int ManageMailbox



