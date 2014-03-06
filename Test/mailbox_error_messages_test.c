/**
 * Try to invoke all the possible ways to get an error
 * @author Khazhismel Kumykov
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
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "mailbox.h"

#define IGNORE -123124

/**
 * Macro to print out test name and result, yay
 * @param  name [description]
 * @return      [description]
 */
#define do_test(name) \
switch(name()) {\
    case IGNORE:\
        break;\
    case 0:\
        printf("FAILED - Test %s\n",#name);\
        exit(1);\
    default:\
        printf("PASSED - Test %s\n",#name);\
    }

/**
 * macro to return if false, print the reason
 * @param  val [description]
 * @return     [description]
 */
#define expect_true(val) \
    if (!(val)) {\
        printf("Failed expect_true for %s\n", #val);\
        return 0;\
    }

#define log(...) \
    if (verbose) {\
        printf(__VA_ARGS__);\
    }

static int verbose = 0;

/**
 * Function to fork and wait for that child, allows creating a new mailbox without having to run another command
 * @return [description]
 */
int re_fork() {
    pid_t child = fork();
    if (child) {
        int status;
        waitpid(child, &status, 0);
        exit(WEXITSTATUS(status));
    }
}

int bad_process_id(void) {
    int error;
    log("Trying to send message to process 0\n");
    error = SendMsg(0, "Hello", 6, BLOCK);

    return error == MAILBOX_INVALID;
}

int mailbox_full(void) {
    int i, error;
    pid_t curpid = getpid();

    log("Sending messages to fill up mailbox, assuming mailbox size is 32\n");
    for (i = 0; i < 32; i++) {
        log("Sending message... ");
        error = SendMsg(curpid, "Hello", 6, NO_BLOCK);
        if (error) {
            log("Failed\n");
            return false;
        } else {
            log("Successful\n");
        }
    }

    log("Sending another message, this should return mailbox full. Test fails if it doesn't\n");
    error = SendMsg(curpid, "Hello", 6, NO_BLOCK);
    return error == MAILBOX_FULL;
}

int mailbox_empty(void) {
    int i, error, len;
    pid_t sender;
    void* msg = malloc(MAX_MSG_SIZE);

    log("Emptying mailbox from previous test, there should be 32 messages\n");
    for (i = 0; i < 32; i++) {
        log("Removing message... ");
        error = RcvMsg(&sender,msg,&len, NO_BLOCK);
        if (error) {
            free(msg);
            log("Failed\n");
            return false;
        } else {
            log("Successful\n");
        }
    }

    log("Recieving another message, this should return mailbox empty. Test fails if it doesn't\n");
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
        log("Waiting until our child exits to send them a message\n");
        waitpid(child_pid, NULL, 0);
        log("Sending message, this should return invalid mailbox. Test fails if it doesn't\n");
        error = SendMsg(child_pid, "Hello", 6, BLOCK);
        return (error == MAILBOX_INVALID);
    } else {
        log("Child exited\n");
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
int recieve_messages_even_after_stopped() {
    pid_t me = getpid(), you;
    int i = 15, j, k, len, error;

    error = SendMsg(me, &i, sizeof(int), NO_BLOCK);
    expect_true(error == 0);

    error = ManageMailbox(true, &j);
    expect_true(error == 0);
    expect_true(j == 1);

    error = RcvMsg(&you, &k, &len, NO_BLOCK);
    expect_true(error == 0);
    expect_true(k == i);
    expect_true(len == sizeof(int));
    expect_true(me == you);

    error = RcvMsg(&you, &k, &len, NO_BLOCK);
    expect_true(error == MAILBOX_STOPPED);

    return true;
}

void* useless_thread(void* args) {
    return args;
}

int closing_thread_does_not_stop_or_destroy_mailbox() {
    pthread_t thread;
    int error, count;

    error = SendMsg(getpid(), "Hello", 6, NO_BLOCK);
    expect_true(error == 0);
    pthread_create(&thread, NULL, useless_thread, NULL);
    pthread_join(thread, NULL);

    ManageMailbox(false, &count);
    expect_true(count == 1);

    return true;
}

void* thread_that_spams_messages(void* args) {
    int* arg = (int*) args;
    int error = 0;
    while (error != MAILBOX_INVALID) {
            log("I'm %d, thread %d and I'm trying to SendMsg, last error %d\n", getpid(), pthread_self(), error);
            error = SendMsg(*arg, "Hello", 6, NO_BLOCK);
    }
    log("I'm %d, thread %d, final error %d\n", getpid(), pthread_self(), error);

    pthread_exit(0);
}

void* thread_that_recieves_messages(void* args) {
    printf("I'm a thread\n");
    int* arg = (int*) args;
    int error = 0;
    printf("I'm a thread\n");
    while (error != MAILBOX_STOPPED) {
            printf("I'm %d, thread %d and I'm trying to SendMsg, last error %d\n", getpid(), pthread_self(), error);
            error = SendMsg(*arg, "Hello", 6, NO_BLOCK);
    }
    printf("I'm %d, thread %d, final error %d\n", getpid(), pthread_self(), error);

    pthread_exit(0);
}

/**
 * Hopefully this can invoke the Mailbox dereference race condition
 *
 * This invokes the pointer dereference race condition every once in a while, not tthe best test...
 * a better test would fork and rerun this several times 
 *
 * If this test fails, you get a kernel oops
 * @return [description]
 */
int rapid_fire_send_and_throw_an_exit_in_there() {
    pid_t parent = getpid(),
          child = fork();
    int error, i;

    if (child) {
        pthread_t threads[100];
        int num_threads = 100;
        //printf("Creating threads for %d\n", getpid());
        for (i = 0; i < num_threads; i++) {
            int error = pthread_create(&threads[i], NULL, &thread_that_spams_messages, &child);
            if (error) {
                printf("Error creating threads, only made %d threads, %s\n", i, strerror(error));
                num_threads = i;
                break;
            }
        }

        for (i = 0; i < num_threads; i++) {
            //printf("joining thread #%d\n", i);
            error = pthread_join(threads[i], NULL);
            //printf("thread joined, error %s\n", strerror(error));
        }

        waitpid(child,NULL,0);
        //printf("Mean thrad read\n");
        return 1;
    } else {
        // child sleeps a little bit then exits, hopefully eventaully provoking that race condition
        
        usleep(500000);
        //printf("Main child exiting %d\n", getpid());
        exit(0);
    }
}

/**
 * Stress test sending and recieving messages, one way
 * hypothetically, two way is the same thing, just need to create two threads in each process
 *
 * Softlocks sometimes - and it the weirdest spots? Every time it locks up, the program counter is on
 * _raw_spin_unlock for one of the processors, like it gets locked up while trying to unlock
 * @return [description]
 */
int rapid_fire_send_recieve_track_how_many_messages_we_get_eventaully() {
    pid_t parent = getpid(),
        child = fork();

    struct timeval five_seconds;
    five_seconds.tv_sec = 1;
    five_seconds.tv_usec = 0;

    log("Having child send as many messages as possible to parent, recording how many messages sent\n");
    log("Parent records how many messages recieved\n")
    if (!child) {
        unsigned long long success = 0;
        struct timeval time_now, end_time;
        gettimeofday(&time_now, NULL);
        timeradd(&time_now, &five_seconds, &end_time);
        log("Hi, I'm the child, sending as many messages as I can in %ld seconds\n", five_seconds.tv_sec);
        while(timercmp(&time_now, &end_time, <)) {
            gettimeofday(&time_now, NULL);
            if (!SendMsg(parent, "Hello", 6, NO_BLOCK)) {
                log("Sent message successfully\n");
                success++;
            }
        }


        // Lets use our message passage system for something useful :OOO
        char result_message[MAX_MSG_SIZE];

        int length_of_message = snprintf(result_message, MAX_MSG_SIZE, "%s %lld", "FINAL_RESULT:", success + 1);
        if (!SendMsg(parent, result_message, length_of_message + 1, BLOCK)) {
            success++;
        }
        log("Sent %llu messages successfully\n", success);
        exit(0);
    } else {
        int status, len;
        unsigned long long success = 0,
                            result;
        pid_t sender;
        void* msg = malloc(MAX_MSG_SIZE);
        log("Parent here, recieving messages!\n");
        while (!waitpid(child, &status, WNOHANG)) {
            if(!RcvMsg(&sender, msg, &len, NO_BLOCK)) {
                log("Got message from %d: %s\n", sender, (char*)msg);
                // len > 15 because most messages are less than that, we don't wnat to have to run strncmp a lot
                if (len > 15 && strncmp((char*)msg,"FINAL_RESULT:", 13) == 0) {
                    log("Got final result message!\n");
                    sscanf(msg, "FINAL_RESULT: %lld", &result);
                    log("Result message: %lld\n", result);
                }
                success++;
            }
        }

        ManageMailbox(true, &len);

        log("Emptying mailbox\n");
        while (!RcvMsg(&sender, msg, &len, BLOCK)) {
            log("Got message from %d: %s\n", sender, (char*)msg);
            // len > 15 because most messages are less than that, we don't wnat to have to run strncmp a lot
            if (len > 15 && strncmp((char*)msg,"FINAL_RESULT:", 13) == 0) {
                log("Got final result message!\n");
                sscanf(msg, "FINAL_RESULT: %lld", &result);
                log("Result message: %lld\n", result);
            }
            success++;
        }

        expect_true(success == result);

        free(msg);
        log("Recieved %llu messages successfully\n", success);

        return 1;
    }
}


struct thread_args {
    int num;
    pid_t* pids;
};

void* thread_that_randomly_sends_messages(void* args) {
    int messages_to_send = (drand48())*1000 + 1;
    int i;
    int success = 0;
    int full = 0;
    int stopped = 0;
    int othererror = 0;

    struct thread_args *ar = (struct thread_args* )args;
    log("Going to makssse %d threads\n", messages_to_send);
    log("I know about %d threads\n", ar->num);

    for (i = 0; i < messages_to_send; i++) {
        int block = (int)(drand48()+0.5);
        int who = (int)(drand48()*ar->num);
        log("sending to %d\n",ar->pids[who]);

        int error = SendMsg(ar->pids[who], "Hello, I am a thread", 21, block);
        switch(error) {
            case MAILBOX_FULL:
                log("full\n");
                full++;
                break;
            case MAILBOX_STOPPED:
                log("send stopped\n");
                stopped++;
                break;
            case 0:
                log("send success\n");
                success++;
                break;
            default:
                log("send othereror\n");
                othererror++;
        }
        usleep(drand48()*100);
    }

    printf("I sent:\n");
    printf("Successfully %d times\nFull mailbox %d times\nStopped mailbox %d times\nOther error %d times\n",success,full,stopped,othererror);
    pthread_exit(0);
}

void* thread_that_randomly_recieves_messages(void* args) {
    int messages_to_rcv = (drand48())*1000 + 1;
    int i;
    int success = 0;
    int empty = 0;
    int stopped = 0;
    int othererror = 0;
    printf("Going to make %d threads\n", messages_to_rcv);
    printf("rcvniog for %d\n",getpid());

    for (i = 0; i < messages_to_rcv; i++) {
        int block = (int)(drand48()+0.5);
        pid_t who;
        char msg[MAX_MSG_SIZE];
        int len;

        int error = RcvMsg(&who,msg, &len, block);
        switch(error) {
            case MAILBOX_EMPTY:
                log("empty\n");
                empty++;
                break;
            case MAILBOX_STOPPED:
                log("rcv, stopped\n");
                stopped++;
                break;
            case 0:
                log("rcv success\n");
                success++;
                break;
            default:
                log("rcv, othererr\n");
                othererror++;
        }
        usleep(drand48()*100);
    }

    printf("I recieved:\n");
    printf("Successfully %d times\nEmpty mailbox %d times\nStopped mailbox %d times\nOther error %d times\n",success,empty,stopped,othererror);

    pthread_exit(0);
}

#define NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST 8
/**
 * the name is pretty descriptive
 * @return [description]
 */
int the_crazy_test_that_is_suggested_in_the_pdf_handout() {
    pid_t parent_pid = getpid();
    int child = 0,
        count = NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST,
        i;
    pid_t pids[NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST];
    for (i = 0; i< NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST; i++) {
        pids[i] = 0;
    }

    // Fork a ton of processes;
    do {
        child = fork();
        pids[NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST-count] = child;
    } while (child && --count > 0);
    log("I am %d, my parent is %d.\n", getpid(), parent_pid);

    int num_i_know_about;
    for (i = 0; i < NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST; i++) {
        if (pids[i]) {
            log("I, %d, also know about %d\n",getpid(), pids[i]);
            num_i_know_about++;
        }
    }

    pid_t* pids_send = malloc((num_i_know_about+1) * sizeof(pid_t));
    int count_2;
    pids_send[0] = parent_pid;
    for (i = 0; i < NUM_PROCESSES_TO_SPAWN_FOR_THIS_TEST; i++ ) {
        if (pids[i]) {
            count_2++;
            pids_send[count_2] = pids[i];
        }
    }
    
    struct thread_args *targs = malloc(sizeof(struct thread_args));
    targs->num = count_2;
    targs->pids = pids_send;

    srand((long)pids_send);
    srand48((long)pids_send);

    int threads_to_create = (drand48() * 100 + 1)*2;
    log("Making %d threads\n", threads_to_create);

    pthread_t* thrds = malloc(sizeof(pthread_t*)*threads_to_create);
    for (i = 0; i < threads_to_create/2; i = i + 2) {
        if (pthread_create(&thrds[i], NULL, &thread_that_randomly_sends_messages, targs)) {
            printf("Thread creation failed uh oh\n");
            threads_to_create = i-1;
            break;
        }
        if(pthread_create(&thrds[i+1], NULL, &thread_that_randomly_recieves_messages, NULL)) {
            printf("Thread creation failed uh oh\n");
            threads_to_create = i;
            break;
        }
    }

    for (i = 0; i < threads_to_create; i++) {
        if(!pthread_join(thrds[i], NULL)) {
            printf("Failed thread join uh oh\n");
        }
    }

    free(thrds);
    free(targs);
    free(pids_send);

    return 1;
}


int c_supports_bitwise_and_right() {
    int flags1 = 0xff;
    int flags2 = 0x04;
    int flags3 = 0x40;

    expect_true(flags1 & flags2);
    expect_true(!(flags2 & flags3));

    return 1;
}

int main(int argc, char** argv) {
    if (argc == 2) {
        verbose = true;
    }
    
    do_test(c_supports_bitwise_and_right);
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
    do_test(recieve_messages_even_after_stopped);
    re_fork(); // stopped mailbox
    do_test(closing_thread_does_not_stop_or_destroy_mailbox);
    do_test(rapid_fire_send_and_throw_an_exit_in_there);
    re_fork();
    do_test(rapid_fire_send_recieve_track_how_many_messages_we_get_eventaully);
    re_fork();
    do_test(the_crazy_test_that_is_suggested_in_the_pdf_handout);

    return 0;
}