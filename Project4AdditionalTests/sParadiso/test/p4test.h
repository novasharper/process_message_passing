
//---------------------------------------------------------------------------
// Susan Paradiso
// CS502
// Project 4
//
//---------------------------------------------------------------------------

#ifndef __SUSAN_P4TEST_DEFINE__
#define __SUSAN_P4TEST_DEFINE__

#include <stdbool.h>
#include <unistd.h>
#include <linux/types.h>
#include <pthread.h>

#include "mailbox.h"


//---------------------------------------------------------------------------
// defines
//---------------------------------------------------------------------------
#define MAX_ERR_SZ    128
#define TEST_PASSED   "\nTEST PASSED\n\n"

#define NULL_IDX      (0-1)
#define NULL_PID      (0-1)

#define REQ_ACK       "REQ_ACK"
#define ACK           "ACK"


//---------------------------------------------------------------------------
// structs/typedefs
//---------------------------------------------------------------------------

typedef enum too_long_mode {
	SLOW_MODE_ON,
	SLOW_MODE_OFF,
	TOO_LONG
} too_long_mode_e;

typedef struct test_info {
	unsigned int test_num;
} test_info_t;

typedef struct err_info {
	int  act_code;
        char act_msg[MAX_ERR_SZ];
	char exp_msg[MAX_ERR_SZ]; 
} err_t;

typedef struct msg_t {
	int   len;
	pid_t pid;
	bool  is_string; //set to true if msg is a string
	char  buf[MAX_MSG_SIZE];
	bool  block;
} msg_t;

typedef struct dest_s {
	pid_t           pid;
	unsigned int    msg_open;
} dest_t;

typedef struct tc_args_s {
	pthread_mutex_t dest_lock;
	dest_t          *dest; //pointer to list of pids we know about
        unsigned int    num_dests;

        pid_t           master_pid;
	unsigned int    num_msgs;
	bool            im_a_child; //alternative is a pthread

	// --- exer only features:
	//args from user:
	unsigned int    num_levels;
	unsigned int    max_num_msgs;    //per sender
	unsigned int    max_num_kids;    //children per level
	unsigned int    max_num_threads; //threads per child

	//dynamic features (change per child and/or thread);
	unsigned int    level;
	unsigned int    num_kids;
	unsigned int    num_threads;
        pid_t           parent_pid; //null for top process
	bool            stop_exer; 
	unsigned int    num_threads_running; //decrs as they end

} tc_args_t;

typedef struct ex_args_s {
	tc_args_t      *tc_args;
	unsigned int   my_idx;
} ex_args_t;


//---------------------------------------------------------------------------
// globals
//---------------------------------------------------------------------------
pthread_mutex_t     print_lock;
bool                this_mbox_stopped;


//---------------------------------------------------------------------------
// prototypes
//---------------------------------------------------------------------------

int  RdOneMsg(msg_t *msg, int expected_err, char *prefix);
void RdGdMsgsAndExitAfterXMsgs(unsigned int num_msgs, bool msg_is_str, 
		               bool stop_first);
int  SendOneMsg(msg_t *msg, int exp_err);
int  SendCharMsg(pid_t pid, bool block, char *str, int exp_err);

void InitFullCharMsg(msg_t *msg, pid_t pid, bool block, char *str);
void InitCharMsg(msg_t *msg, pid_t pid, bool block);

void TestGdMsgs();
void TestBadSendArg();
void TestBadRcvArg();
int  main(int argc, char **argv);

#endif

