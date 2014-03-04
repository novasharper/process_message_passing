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
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#include "mailbox.h"

#define IGNORE -123124

#define do_test(name) \
switch(name()) {\
    case IGNORE:\
        break;\
    case 0:\
        printf("Test %s failed!\n",#name);\
        return 1;\
        break;\
    default:\
        printf("Test %s passed!\n",#name);\
    }

#define expect_true(val) \
    if (!(val)) {\
        printf("Failed expect_true for %s\n", #val);\
        return 1;\
    }

int re_fork() {
    pid_t child = fork();
    if (child) {
        int status;
        waitpid(child, &status, 0);
        exit(status);
    }
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
            free(msg);
            return false;
        }
    }

    error = RcvMsg(&sender,msg,&len, NO_BLOCK);
    free(msg);
    return error == MAILBOX_EMPTY;
}

int mailbox_exited(void) {
    int i, error;
    pid_t parent = getpid();

    int child_pid = fork();
    if (child_pid) {
        // parent
        waitpid(child_pid, NULL, 0);
        error = SendMsg(child_pid, "Hello", 6, BLOCK);
        return (error == MAILBOX_INVALID);
    } else {
        exit(0);
    }
}

int mailbox_stopped(void) {
    int i, error;
    pid_t me = getpid();
    void* msg = malloc(MAX_MSG_SIZE);

    ManageMailbox(true, &i);

    error = SendMsg(me, "Hello", 6, BLOCK);
    if (error != MAILBOX_STOPPED) {
        free(msg);
        return false;
    }

    error = RcvMsg(&me, msg, &i, BLOCK);
    free(msg);
    return (error == MAILBOX_STOPPED);
}

int blocked_wait_then_stopped() {
    pid_t child = fork();
    int error;

    if (child) {
        int i;
        for (i = 0; i < 32; i++) {
            SendMsg(child, "Hello", 6, NO_BLOCK);
        }
        error = SendMsg(child, "Hello", 6, BLOCK);
        return (error == MAILBOX_STOPPED);
    } else {
        usleep(5000);
        ManageMailbox(true, &error);
        exit(0);
    }
}

void * subthread_dorecieve(void* args) {
    pid_t sender;
    void* msg = malloc(MAX_MSG_SIZE);
    int len;

    int* error = malloc(sizeof(int));

    *error = RcvMsg(&sender, msg, &len, BLOCK);
    free(msg);

    return error;
}

int blocked_wait_rcv_then_stopped() {
    pthread_t subt;
    int count;
    void* err;
    pthread_create(&subt, NULL, subthread_dorecieve, NULL);
    ManageMailbox(true, &count);
    pthread_join(subt, &err);

    int error = *(int*)err;
    free(err);

    return error == MAILBOX_STOPPED;
}

int msg_arg_error_invoke() {
    int error1 = ManageMailbox(false, NULL); // can't write to NULL, fail
    expect_true(error1 == MSG_ARG_ERROR);

    int error1_5 = ManageMailbox(true, NULL); // can't write to NULL, fail. Malformed command does not stop mailbox
    expect_true(error1_5 == MSG_ARG_ERROR);

    int error2 = SendMsg(getpid(),NULL, 6, NO_BLOCK); //can't read 6 from null, fail
    expect_true(error2 == MSG_ARG_ERROR);

    int error3 = SendMsg(getpid(),NULL,0,NO_BLOCK); // can read 0 from null, pass
    expect_true(error3 == 0);

    int error4 = RcvMsg(NULL, NULL, NULL, NO_BLOCK);    // can't send null to null, fail
    expect_true(error4 == MSG_ARG_ERROR);

    pid_t sender;
    char msg[MAX_MSG_SIZE];
    int len;
    int error5 = RcvMsg(NULL, msg, &len, NO_BLOCK); // can't read to null, fail
    expect_true(error5 == MSG_ARG_ERROR);

    int error6 = RcvMsg(&sender, msg, NULL, NO_BLOCK);  // can't read to null, fail
    expect_true(error6 == MSG_ARG_ERROR);

    int error7 = RcvMsg(&sender, msg, &len, NO_BLOCK);  // can read to all, pass
    expect_true(error7 == 0);
    expect_true(len == 0);

    int len2;
    SendMsg(getpid(),"Hello",6,NO_BLOCK);
    int error8 = RcvMsg(&sender, NULL, &len2, NO_BLOCK);    // can't read to null, len isn't 0, fail
    expect_true(error8 == MSG_ARG_ERROR);

    return true;
}

int msg_len_errors() {
    pid_t me = getpid();
    char msg1[MAX_MSG_SIZE+1];
    char msg2[MAX_MSG_SIZE];
    int i;
    for (i = 0; i < MAX_MSG_SIZE; i++) {
        msg2[i] = 'g' + i;
    }
    char msg2_rcv[MAX_MSG_SIZE];
    char msg3[1];
    msg3[0] = 'h';

    int error = SendMsg(me,msg1,MAX_MSG_SIZE+1,NO_BLOCK);
    expect_true(error == MSG_LENGTH_ERROR);

    error = SendMsg(me,msg2,MAX_MSG_SIZE,NO_BLOCK);
    expect_true(error == 0);

    int len;
    error = RcvMsg(&me, msg2_rcv, &len, NO_BLOCK);
    expect_true(error == 0);
    expect_true(me == getpid());
    expect_true(len == MAX_MSG_SIZE);
    for (i = 0; i < MAX_MSG_SIZE; i++) {
        expect_true(msg2[i] == msg2_rcv[i]);
    }

    error = SendMsg(me, msg3, 1, NO_BLOCK);
    expect_true(error == 0);

    char msg3_rcv[MAX_MSG_SIZE];
    error = RcvMsg(&me, msg3_rcv, &len, NO_BLOCK);
    expect_true(len == 1);
    expect_true(len != 2);
    expect_true(msg3_rcv[0] == msg3[0]);

    error = SendMsg(me, "Hello", -1, NO_BLOCK);
    expect_true(error = MSG_LENGTH_ERROR);

    return true;
}

/**
 * Tests that we are recieveing messages in first-in-first-out, even if there's an error while reading once.
 * @return [description]
 */
int fifo_even_if_errors() {
    pid_t parent = getpid(), child = fork();

    if (child) {
        // in parent
        usleep(50000);

        pid_t sender;
        int msg, len, i, error;
        error = RcvMsg(NULL, NULL, NULL, BLOCK);
        expect_true(error == MSG_ARG_ERROR);

        for (i = 0; i < 32; i++) {
            error = RcvMsg(&sender,&msg,&len,BLOCK);
            expect_true(error == 0);
            expect_true(len == sizeof(int));
            //printf("expected %d, got %d\n", i, msg);
            expect_true(msg == i);
        }
    } else {
        // in child
        int i;
        for (i = 0; i < 32; i++) {
            SendMsg(parent, &i, sizeof(int), NO_BLOCK);
        }
        exit(0);
    }
}

int main(void) {
    do_test(bad_process_id);
    do_test(mailbox_exited);
    do_test(mailbox_stopped);
    re_fork();  // we stopped our mailbox, get a new one
    do_test(mailbox_full); // must run before mailbox empty
    do_test(mailbox_empty); // must run immediately after mailbox full
    do_test(blocked_wait_then_stopped);
    do_test(blocked_wait_rcv_then_stopped);
    re_fork(); // we stopped our mailbox, get a new one
    do_test(msg_arg_error_invoke);
    re_fork(); // flush old mailbox
    do_test(msg_len_errors);
    do_test(fifo_even_if_errors);

    return 0;
}