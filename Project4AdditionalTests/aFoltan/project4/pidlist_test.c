/*
 * pidlist_test.c
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
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pidlist.h"

#define NUM_PROCESSES   10
#define TEST_TIMEOUT    10

#define UP_TO_250_MSEC   (rand()&0x3FFFF)

#define SEED()   { \
	struct timeval seedtime; \
	gettimeofday( &seedtime, NULL ); \
	srand( seedtime.tv_usec  ); }

#define DEBUG  false

/*
 *  do_pidlist_test
 *
 *  A routine to randomly add and remove PIDs from
 *  the shared memory PID linked list.
 *
 */
void do_pidlist_test( int timeout ) {

	pid_t pid;

	time_t quittime = time(NULL);
	quittime += timeout;

	printf("PROCESS = %u\n",getpid());

	sleep(1);

	do {

		switch(rand()&0x7) {
		default:
		case 0:
		case 1:
		case 2:
			pid = (pid_t)rand();
			pidlist_add_pid( pid );
			break;
		case 3:
		case 4:
		case 5:
			pid = pidlist_get_random();
			pidlist_remove_pid( pid );
			break;
		case 6:
			pid = (pid_t)rand();
			pidlist_remove_pid(pid);
			break;
		case 7:
			pidlist_get_count();
			break;
		}

		usleep( 5000 );

	} while ( time(NULL) < quittime );

	printf("PROC %u done.\n",getpid());
}

/*
 *   new_process
 *
 *   Spawns a new process to join the test
 */
void new_process( int timeout ) {

	pid_t pid;

	pid = fork();

	if (pid < 0) {
		printf("FATAL:: fork() failed\n");
		exit(-1);
	} else if (pid == 0) {			// child process

		print_lock_info();

		// set new seed for random number generation
		SEED();

		do_pidlist_test( timeout );

		exit(0);
	}

	printf("[%u] leaving new_process()\n", getpid());

}


/*
 *  pidlist_test
 *
 *  Test code to specifically exercise the shared memory implementation
 *  of the PID list.  This code spawns multiple processes which randomly
 *  add and remove process IDs from the shared memory linked list to
 *  ensure no race conditions or leaks occur.
 *
 *  This code is used to verify the PID list management outside and prior
 *  to use in the mailbox testing.
 *
 */
int main( int argc, char **argv ) {

	int i;
	pid_t child;
	int proc_count = 0;

	if (pidlist_init() != 0) {
		printf("Failed to initialize the PID list manager!\n");
		exit(-1);
	}

	printf("pidlist_init() success\n");
	fflush(stdout);

	for(i=0;i<NUM_PROCESSES;i++) {
		new_process( TEST_TIMEOUT );
		printf("[%u] COUNT = %d\n",getpid(),++proc_count);
	}

	while( (child = wait(NULL))>0 || errno != ECHILD ) {
		printf("[%u] COUNT = %d, PROCESS=%d, errno=%d\n",getpid(),proc_count--,child,errno);
	}

	printf("Freeing all remaining PIDs:\n");
	while (pidlist_get_count() > 0) {
		pidlist_remove_pid(pidlist_get_first());
	}

	fflush(stdout);

	return 0;
}

