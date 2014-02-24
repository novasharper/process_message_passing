//Michael Sanchez
//CS 502 - Cisco
//mailboxTest.c

#include "mailbox.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#define NUM_PROC_LEVELS 2           //Number of levels in process tree

#define AVG_PROCS_PER_LEVEL 2       //Average number of processes to spawn at each level
#define AVG_THREADS_PER_PROC 2      //Average number of threads to spawn for each process
#define AVG_CMD_DELAY 10            //Average delay between commands in ms
#define AVG_CMDS 100                //Average number of commands each thread will do

#define WEIGHT_SEND 500             //The weight to do a send message 
#define WEIGHT_RECEIVE 100           //The weight to receive a single message
#define WEIGHT_RECEIVE_ALL 50        //The weight to receive all messages
#define WEIGHT_MANAGE_STOP 1        //The weight to stop the mailbox
#define WEIGHT_EXIT 1               //The weight to exit the mailbox

#define SEND_BLOCK false
#define RECEIVE_BLOCK false

int parent;                         //Process we start everything from...used to prevent us from learning about it
struct drand48_data drand_seed;     //Per processes random # seed

struct node {
    struct node *next;
    pid_t pid;
};

int genRandUniform(int mean) {
    double a, r;
    
    //Get Random number;
    drand48_r(&drand_seed, &a);

    //Scale out so number is 0 to 2 x MEAN
    r = a * (2 * mean) + 1;
    return (int)r;
}

int getRandKnown(struct node *known) {
    int count, rand, i;
    struct node* iterator = known;

    count = 0;
    while (iterator != NULL) {
        count++;
        iterator = iterator->next;
    }
    //No items, return ourself...
    if (count == 0)
        return getpid();

    //Generate a rand, and reset the iterator
    rand = genRandUniform(count);
    iterator = known;
    for (i = 0; i < rand; i++) {
        if (iterator->next == NULL)
            iterator = known;
        else
            iterator = iterator->next;
    }
    return iterator->pid;
}

//Add to our list of known pids to send to.
struct node* addKnown(pid_t pid, int id, struct node *known) {
    struct node* iterator = known;
    struct node* new;
    
    //Lets not add init as a known proc
    if (pid == 1 || pid == 0 || pid == parent || pid == getpid()) 
        return known;

    //Make sure we aren't already in the list 
    //And get to the end
    while (iterator != NULL && iterator->next != NULL) {
        //If we are looking at the pid we are trying
        //to add already, break the loop
        if (iterator->pid == pid) {
            break;
        }
        //Otherwise, loop to next
        iterator = iterator->next;
    }
    //If the iterator ended on our PID, we don't want to add
    if (iterator != NULL && iterator->pid == pid)
        return known;
    //Otherwise, iterator is now also the last entry
    new = (struct node*)malloc(sizeof(struct node));   
    new->pid = pid;
    new->next = NULL;
    
    printf("LEARN: %d-%d has learned of %d\n", getpid(), id, pid);
    //Case of first add
    if (iterator == NULL)
        return new;
    else {
        iterator->next = new;
        return known;
    }
}

struct node* removeKnown(pid_t pid, int id, struct node *known) {
    struct node* curr = known;
    struct node* prev = NULL;

    printf("UNLEARN: %d-%d is unlearning %d\n", getpid(), id, pid);    
    //Known was null (no list)
    if (curr == NULL)
    {
        return NULL;
    }

    //Cycle until we find our pid
    while (curr->pid != pid) {
        //If we hit the end, break out
        if (curr == NULL) {
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    //Couldn't find it case
    if (curr == NULL) {
        return known;
    }

    //We found it, now we need to remove it.
    if (prev == NULL) {
        //First element was our pid to remove
        //Jump ahead to save our new head
        prev = curr->next;
        //Free the first element
        free(curr);
        //Return a pointer to the second element
        //This will be null if there was only 
        //One element
        return prev;
    } else {
        //Somewhere in the middle or end
        prev->next = curr->next;
        free(curr);
        return known;
    }
}

char * getReturnString(int ret) {
    if (ret == 0) {
        return "Success";
    } else if (ret == 1001) {
        return "Mailbox full";
    } else if (ret == 1002) {
        return "Mailbox empty";
    } else if (ret == 1003) {
        return "Mailbox stopped";
    } else if (ret == 1004) {
        return "Invalid Mailbox";
    } else if (ret == 1005) {
        return "Message too long";
    } else if (ret == 1006) {
        return "Argument error";
    } else if (ret == 1007) {
        return "Mailbox error";
    } 
    return "Uknown Return Code";
}

void  *doMailbox(void* tid) {
    struct node *known; //Our list of nodes for PID's that we know about
    struct node *curr;  //Current position while looping over known
    int i, num_cmds, cmd, ret;
    int total_weight = WEIGHT_SEND + WEIGHT_RECEIVE + WEIGHT_RECEIVE_ALL +
                       WEIGHT_MANAGE_STOP + WEIGHT_EXIT;
    int id = (int)tid;
    //Variables to use with syscalls
    pid_t sender;
    void *msg[MAX_MSG_SIZE];
    int s_pid, r_pid;
    int length, count, j;

    //Init our known list and iterator
    known = NULL;
    known = addKnown(getppid(), id, known);
    curr = NULL;

    printf("Thread %d in PID/TID %d/%ld starting\n", id, getpid(), syscall(224));

    num_cmds = genRandUniform(AVG_CMDS);
    for (i = 0; i < num_cmds; i++) {
        usleep(genRandUniform(AVG_CMD_DELAY) * 1000);
        cmd = genRandUniform(total_weight);
        if (cmd < 2 * (WEIGHT_SEND)) {
            //Send to our next known
            //Get the person to send to
            s_pid = getRandKnown(known);
            if (s_pid == getpid()) {
                printf("SEND: Unable to send message in %d-%d - No known pids\n", getpid(), id);
                continue;
            }
            r_pid = getRandKnown(known);
            ret = SendMsg(s_pid, (void *)&r_pid, sizeof(int), SEND_BLOCK);
            printf("SEND: Sent message from %d-%d to %d: %d\n", getpid(), id, s_pid, r_pid);
            //If we couldn't send because the proc is gone, or the mailbox is stopped
            //Remove it so we no longer send to it.
            if (ret == 1003 || ret == 1004) {
                printf("SEND: Sent message from %d-%d to %d failed: %d \n", getpid(), id, s_pid, r_pid);
                known = removeKnown(s_pid, id, known);
            }

        } else if (cmd < 2 * (WEIGHT_SEND + WEIGHT_RECEIVE)) {
            //Receive 1 message
            //ret = RcvMsg(&sender, msg, &length, false);
            ret = RcvMsg(&sender, msg, &length, RECEIVE_BLOCK);
            if (ret) 
                printf("RECV: Unable to receive message in %d-%d - %s\n", getpid(), id, getReturnString(ret) );
            else { 
                known = addKnown(sender, id, known);
                if (length != 4) {
                    printf("RECV: Bad length received\n");
                    continue;
                }
                r_pid = *(int *)msg;
                known = addKnown(r_pid, id, known);
                printf("RECV: Recieved messaged from %d in %d-%d: %d\n", sender, getpid(), id, r_pid);
            }
                
        } else if (cmd < 2 * (WEIGHT_SEND + WEIGHT_RECEIVE + WEIGHT_RECEIVE_ALL)) {
            //Receive ALL
            ret = ManageMailbox(false, &count);
            if (ret) {
                printf("Problem receiving all messages in %d-%d - %s\n", getpid(), id, getReturnString(ret));
                continue;
            }
            printf("RECV ALL: %d-%d receiving %d messages\n", getpid(), id, count);
            for (j = 0; j < count; j++) {
                //ret = RcvMsg(&sender, msg, &length, false);
                ret = RcvMsg(&sender, msg, &length, RECEIVE_BLOCK);
                if (ret)
                    printf("RECV ALL: Unable to receive message in %d-%d - %s\n", getpid(), id, getReturnString(ret) );
                else { 
                    known = addKnown(sender, id, known);
                    if (length != 4) {
                        printf("Bad length received\n");
                        continue;
                    }   
                    r_pid = *(int *)msg;
                    known = addKnown(r_pid, id, known);
                    printf("RECV ALL: Recieved messaged from %d in %d-%d: %d\n", sender, getpid(), id, r_pid);
                }
            }
        } else if (cmd < 2 * (WEIGHT_SEND + WEIGHT_RECEIVE + WEIGHT_RECEIVE_ALL + 
                          WEIGHT_MANAGE_STOP)) {
            //Stop Mailbox
            ret = ManageMailbox(true, &count);
            if (ret)
                printf("Problem managing mailbox in %d - %s\n", getpid(), getReturnString(ret));
            printf("MANAGE: %d mailbox has been stopped by thread %d\n", getpid(), id);
        } else {
            //Exit now!
            printf("EXIT: Thread exiting early %d-%d\n", getpid(), id);
            break;
        }   
    }
    sleep(10);
    pthread_detach(pthread_self());
    printf("Thread %d-%d exiting\n", getpid(), id);
    pthread_exit(0);
}

int spawnProcs(int current_level) {
    int child_pid;
    int i;
    int num_procs;

    //Make sure we don't go down too far
    if ((current_level + 1) > NUM_PROC_LEVELS) 
        return current_level;

    num_procs = genRandUniform(AVG_PROCS_PER_LEVEL);
    for (i = 0; i < num_procs; i++) {
        child_pid = fork();
        if (child_pid == 0) {
            //Child
            current_level++;    //We are down another level now

            //Recursively spawn processes to build the process tree
            return spawnProcs(current_level);
        } 
    }
    return current_level;
}

int main() {
    int myLevel;
    int num_threads, i;
    struct timeval seedTime;
    pthread_t thread;

    parent = getppid();

    gettimeofday(&seedTime, NULL);
    srand48_r(seedTime.tv_sec * 1000000 + seedTime.tv_usec, &drand_seed);

    //Spawn processes.  My Level has how far down the process tree this process is.
    myLevel = spawnProcs(0);

    //TODO: Process level initialization
    gettimeofday(&seedTime, NULL);
    srand48_r(seedTime.tv_sec * 1000000 + seedTime.tv_usec, &drand_seed);

    sleep(1);

    num_threads = genRandUniform(AVG_THREADS_PER_PROC);
    printf("\nProcess %d is creating %d threads\n", getpid(), num_threads);
    sleep(2);
    for (i = 0; i < num_threads; i++) {
    //Spawn off the threads to do the work
        pthread_create(&thread, NULL, doMailbox, (void*) (i+1));    
    }

    sleep(5);

    printf("Master Thread %d exiting\n", getpid());    
    pthread_detach(pthread_self());
    pthread_exit(0);
}

