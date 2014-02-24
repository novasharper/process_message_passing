/*
 * test7.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <semaphore.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mailbox.h"
#include "test_utils.h"
#include "pidlist.h"

#define DEFAULT_NUM_PROCESSES   100
#define DEFAULT_AVG_THREADS     10
#define DEFAULT_TIMEOUT         10

#define DEBUG  false

struct stats_data {
	int sent;
	int send_full;
	int rcvd;
	int recv_empty;
};

struct shm_region {
	sem_t lock;
};

static struct shm_region *shm = NULL;
static int fd = -1;

#define PLOCK()    sem_wait(&shm->lock)
#define PUNLOCK()  sem_post(&shm->lock)

// command line arguments
static int max_processes = DEFAULT_NUM_PROCESSES;
static int mean_threads = DEFAULT_AVG_THREADS;
static int timeout = DEFAULT_TIMEOUT;

/*
 * do_mailbox_test
 *
 * The primary thread processing loop.  Each thread chooses a random process
 * and sends a message to the peer process.  It then reads it own mailbox to
 * receive a message.  Both sending and receiving are NON-BLOCKING.
 *
 * While sending and receiving, statistics are collected:
 *     	- number of messages successfully sent
 *  	- number of sends to a full mailbox (MAILBOX_FULL)
 *  	- number of messages received
 *  	- number of reads of an empty mailbox (MAILBOX_EMPTY)
 *
 *
 */
void do_mailbox_test( int timeout, struct stats_data * stats ) {

	char msg[MAX_MSG_SIZE];
	int rc;
	int recv_len;
	pid_t sender_pid;
	pid_t dest_pid;

	sleep(1);

	time_t quittime = time(NULL);
	quittime += timeout;

	do {

		dest_pid = pidlist_get_random();
		sprintf( msg, "From: %u-%lu, To: %u", getpid(), pthread_self(), dest_pid );
		rc = SendMsg( dest_pid, msg, strlen(msg)+1, NO_BLOCK );
		switch(rc) {
		case 0:		// success
			stats->sent++;
			if (DEBUG) printf("%u-%lu sent %s\n",getpid(), pthread_self(), msg);
			break;
		case MAILBOX_FULL:
			stats->send_full++;
			if (DEBUG) printf("%u-%lu unable to send to %u due to MAILBOX_FULL\n",getpid(),pthread_self(),dest_pid);
			break;
		case MAILBOX_INVALID:
			printf("SendMsg [%u] %s\n",dest_pid,mailbox_code(rc));
			break;
		default:
			printf("SendMsg error %s when %u-%lu sending to %u\n",mailbox_code(rc),getpid(),pthread_self(),dest_pid);
			FAILURE_EXIT();
		}

		if (DEBUG) usleep( UP_TO_250_MSEC );

		rc = RcvMsg( &sender_pid, msg, &recv_len, NO_BLOCK );
		switch(rc) {
		case 0:		// success
			stats->rcvd++;
			if (DEBUG) printf("%u-%lu rcvd %s\n",getpid(),pthread_self(),msg);
			break;
		case MAILBOX_EMPTY:
			stats->recv_empty++;
			if (DEBUG) printf("%u MAILBOX_EMPTY\n",getpid());
			break;
		case MAILBOX_STOPPED:
			printf("%u unable receive due to MAILBOX_STOPPED\n",getpid());
			break;
		default:
			printf("do_mailbox_test RECV error %d\n",rc);
			FAILURE_EXIT();
		}

		if (DEBUG) usleep( UP_TO_250_MSEC );

	} while ( time(NULL) < quittime );

	PLOCK();
	printf("Process %u, Thread %lu done.\n",getpid(),pthread_self());
	PUNLOCK();
}

/*
 *  do_thread_processing
 *
 *  Each thread paticipates in the mailbox testing and prints
 *  its statistics upon completion.
 */
void * do_thread_processing( void * arg ) {

	struct stats_data stats = { 0, 0, 0, 0 };
	int timeout = *(int*)arg;

	do_mailbox_test( timeout, &stats );

	PLOCK();
	printf("Process [%u], Thread [%lu]:\n",getpid(),pthread_self());
	printf("Sent  : %d\n",stats.sent);
	printf("Rcvd  : %d\n",stats.rcvd);
	printf("Full  : %d\n",stats.send_full);
	printf("Empty : %d\n",stats.recv_empty);
	PUNLOCK();

	return 0;
}

/*
 *  new_process
 *
 *  Spawns the child process with random number of threads.
 *  Synchronizes threads and removes PID from PIDLIST pool on exit.
 */
void new_process( int timeout, int num_threads ) {

	pid_t pid;
	int i;

	pid = fork();

	if (pid < 0) {
		printf("FATAL:: fork() failed\n");
		FAILURE_EXIT();
	} else if (pid == 0) {			// child process

		if (num_threads > 0) {
			pthread_t t[2*mean_threads+1];

			PLOCK();
			printf("Starting Process [%u] with %d threads...\n",getpid(),num_threads);
			PUNLOCK();

			// kick off the threads
			for (i=0;i<num_threads;i++) {
				if (pthread_create ( &t[i], NULL, &do_thread_processing, (void*)&timeout ) != 0) {
					printf("Failed to create thread t[%d]!\n",i);
					FAILURE_EXIT();
				}
			}

			// the child main thread participates too
			do_thread_processing( &timeout );

			for (i=0; i<num_threads; i++) {
				pthread_join( t[i], NULL );
			}
		}

		PLOCK();
		pidlist_remove_pid( getpid() );
		PUNLOCK();

		exit(0);

	} else {						// parent process
		// add the new pid into the mailbox storm
		if (pidlist_add_pid( pid ) == -1) {
			printf("Failed to add PID %u to the global PID list -- %d processes already exist\n",
					pid, pidlist_get_count());
		}
	}

}

/*
 *  print_sync_init
 *
 *  Statistics printf is synchronized to ensusre multi-line output
 *  remains contiguous.  This uses a shared memory semaphore.
 */
void print_sync_init( void ) {

	int rc;

	fd = shm_open("/shmregion", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		printf("Failed to create shared semaphore!\n");
		FAILURE_EXIT();
	}

	if (ftruncate(fd, sizeof(sem_t)) == -1) {
		printf("Failed to ftruncate shared memry!\n");
		FAILURE_EXIT();
	}

	/* Map shared memory object */
	shm = mmap(NULL, sizeof(struct shm_region),
		   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shm == MAP_FAILED) {
		printf("Failed to mmap shared memory!\n");
		FAILURE_EXIT();
	}

	//rc = pthread_mutex_init( &rpid->lock, NULL );
	rc = sem_init( &shm->lock, 1, 1);
	if (rc != 0) {
		printf("sem init failure\n");
		FAILURE_EXIT();
	}

}

/*
 *  process_commandline
 *
 *  Prints usage information and validates command line arguments.
 */
void process_commandline( int argc, char ** argv ) {

	if (argc == 1) {
		return;
	}

	if (argc == 4) {
		max_processes = atoi(argv[1]);
		mean_threads = atoi(argv[2]);
		timeout = atoi(argv[3]);
	}

	printf("Usage:");
	printf("   %s  [<max_processes> <mean_threads> <timeout>]\n", argv[0]);
	printf("        max_processes : the number of processes to spawn\n");
	printf("        mean_threads  : the average number of threads per process\n");
	printf("        timeout       : the time in seconds the test will run\n");
	printf("\n");

	exit(-1);
}

/*
 *  Test7
 *
 *  Spawns processes and threads.  Processes will run for a random
 *  amount of time and quit.  As they quit, new processes with new random
 *  threads are spawned to replace the old processes.  In the end, the total
 *  number of processes and threads are reported.
 */
void Test7( void ) {

	int i, tcount;
	pid_t pid;
	int total_proc_count = 0;
	int total_thread_count = 0;

	print_sync_init();

	TEST_INTRO( "Multi-Process / Multi-Threaded Testing" );

	if (pidlist_init() != 0) {
		printf("Failed to initialize the PID list manager!\n");
		FAILURE_EXIT();
	}

	printf("PID list initialized\n");

	for(i=0;i<max_processes;i++) {
		tcount = randmax(2*mean_threads+1);
		new_process( randmax(timeout), tcount );
		total_proc_count++;
		total_thread_count += tcount;
	}

	printf("%d processes are running for the next %d seconds sending and receiving messages randomly ...\n",
			max_processes, timeout);

	time_t quittime = time(NULL);
	quittime += timeout;

	do {

		pid = wait(NULL);

		if (time(NULL) < quittime) {
			// start a new process to
			tcount = randmax(2*mean_threads+1);
			new_process( randmax(timeout), tcount );
			total_proc_count++;
			total_thread_count += tcount;
		}

	} while ( ((pid>0) || (errno != ECHILD)) || (time(NULL) < quittime) );

	// wait until all processes complete
	while( (wait(NULL)>0) || (errno != ECHILD) );

	printf("\nA total of %d unique processes participated in the test\n",total_proc_count);
	printf("\nA total of %d unique threads participated in the test\n",total_thread_count);

	TEST_CLOSING();

	fflush(stdout);

}

/*
 *  Test7
 *
 *  A multi-process / multi-threaded messaging test.  This program provides
 *  command line support for varying the parameters of the test.  By default,
 *  it will spawn 100 processes, each with an average of 10 threads.
 *
 *  Processes will run for a random amount of time and quit.  As they quit,
 *  new processes with new random threads are spawned to replace the old
 *  processes. The total test time could be up to 2 * TIMEOUT.
 *
 *  In the end, the total number of participating processes and threads are
 *  reported.
 *
 *  Each process registers its PID in a shared memory PIDLIST accessible to
 *  all processes.
 *
 *  Each thread chooses a random process and sends a message to the peer process.
 *  It then reads it own mailbox to receive a message.  Both sending and receiving
 *  are NON-BLOCKING.
 *
 * While sending and receiving, statistics are collected:
 *     	- number of messages successfully sent
 *  	- number of sends to a full mailbox (MAILBOX_FULL)
 *  	- number of messages received
 *  	- number of reads of an empty mailbox (MAILBOX_EMPTY)
 *
 *  Any unexpected return codes from send/receive will cause the program to exit().
 *
 *  NOTE: it is possible and likely that send will occur to invalid PIDs.  This occurs
 *  when the target process ends while the sending process is attempting post the message.
 *  In this case, the SendMsg lookup of the task_struct fails, and SendMsg reports
 *  MAILBOX_INVALID.
 */
int main( int argc, char **argv ) {

	// parse command line
	process_commandline( argc, argv );

	SEED();

	Test7();

	return 0;
}

