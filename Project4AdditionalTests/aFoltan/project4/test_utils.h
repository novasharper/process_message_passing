/*
 * test_utils.h
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#ifndef TEST_UTILS_H_
#define TEST_UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "mailbox.h"

// MAX_QUEUE_SIZE is set to the same value the kernel uses to test the limits
#define MAX_QUEUE_SIZE   32

typedef enum {
	ON_SUCCESS_EXIT,
	ON_FAILURE_EXIT,
	NEVER_EXIT
} expected_result_t;

/**
 *   SILENT / NOISY are used to control the verboseness
 *         of the printf outputs in each function
 */
#define SILENT                true		// no printf outputs
#define NOISY                 false		// printf outputs

#define MBX_BLOCK             true
#define MBX_NON_BLOCKING      false

#define MBX_STOP              true
#define MBX_NO_STOP           false

// Helper macros for console output during testing
#define TEST_INTRO(title)  test_intro( __FUNCTION__, title )
#define TEST_CLOSING()     test_closing( __FUNCTION__ )

#define UP_TO_1_MSEC     (rand()&0x003FF)	// 10 bits (actually   1024 usec)
#define UP_TO_2_MSEC     (rand()&0x007FF)	// 10 bits (actually   2048 usec)
#define UP_TO_125_MSEC   (rand()&0x1FFFF)	// 17 bits (actually 131072 usec)
#define UP_TO_250_MSEC   (rand()&0x3FFFF)	// 18 bits (actually 262144 usec)
#define UP_TO_500_MSEC   (rand()&0x7FFFF)	// 19 bits (actually 524288 usec)

#define SEED()   { \
	struct timeval seedtime; \
	gettimeofday( &seedtime, NULL ); \
	srand( seedtime.tv_usec  ); }

// exit macro on failure to flush output
#define FAILURE_EXIT()     { \
	printf("Test FAILED !!\n"); \
	fflush(stdout); \
	exit (-1); }

/*
 *  Function prototypes useful for testing the mailbox functionailty
 */
extern char * mailbox_code( int rc );
extern void test_intro( const char * test, char * name );
extern void test_closing( const char * test );
extern int recv_msg( char *msg, bool block, bool silent, expected_result_t expected );
extern int send_msg( char * message, bool block, bool silent, expected_result_t expected );
extern int send_msg_to_pid( pid_t send_pid, char * message, bool block, bool silent, expected_result_t expected );
extern void send_recv_verify( char * message, bool silent );
extern void drain_queue( bool silent );
extern void paced_send_recv_verify( char * base_msg, int count, bool silent, int sleep_time );
extern void slow_send( char * msg, int count, bool block, bool silent, expected_result_t expected );
extern void burst_send( char * msg, int burst_count, bool block, bool silent, expected_result_t expected );
extern void burst_recv( int burst_count, bool block, bool silent, expected_result_t expected );
extern void fill_mailbox( char * msg );
extern void verify_mailbox_count( int expected_count, bool stop );
extern int randmax( int max );

#endif /* TEST_UTILS_H_ */
