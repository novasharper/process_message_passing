/**
 * Try to invoke all the possible ways to get an error
 */
/*
long SendMsg(pid_t dest, void *msg, int len, bool block);
long RcvMsg(pid_t *sender, void *msg, int *len, bool block);
long ManageMailbox(bool stop, int *count);


#define MAILBOX_FULL        1001
#define MAILBOX_EMPTY       1002
#define MAILBOX_STOPPED     1003
#define MAILBOX_INVALID     1004
#define MSG_LENGTH_ERROR    1005
#define MSG_ARG_ERROR       1006
#define MAILBOX_ERROR       1007

 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "mailbox.h"

#define DO_TEST(name) \
if (name()) {\
    printf("Test %s passed!\n", #name);\
} else {\
    printf("Test %s failed!\n", #name);\
}

int bad_process_id(void) {
    int error;
    error = SendMsg(0, "Hello", 6, BLOCK);

    return error == MAILBOX_INVALID;
}

int mailbox_full(void) {
    int i, error;
    pid_t curpid = getpid();

    for (i = 0; i < 32; i++) {
        error = SendMsg(curpid, "Hello", 6, NO_BLOCK);
        if (error) {
            return false;
        }
    }

    error = SendMsg(curpid, "Hello", 6, NO_BLOCK);
    return error == MAILBOX_FULL;
}

int mailbox_empty(void) {
    int i, error, len;
    pid_t sender;
    void* msg = malloc(MAX_MSG_SIZE);

    for (i = 0; i < 32; i++) {
        error = RcvMsg(&sender,msg,&len, NO_BLOCK);
        if (error) {
            return false;
        }
    }

    error = RcvMsg(&sender,msg,&len, NO_BLOCK);
    return error == MAILBOX_EMPTY;
}

int main(void) {
    DO_TEST(bad_process_id);
    DO_TEST(mailbox_full);
    DO_TEST(mailbox_empty);
}