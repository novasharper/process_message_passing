
//----------------------------------------------------------------------------
// Susan Paradiso
// CS502
// Project 4 test
//
//----------------------------------------------------------------------------
#include "mailbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

#include "p4utils.h"
#include "p4test.h"
#include "mailbox.h"


//exer defines
#define  MAX_NUM_LEVELS  10
#define  MAX_NUM_KIDS    10
#define  MAX_NUM_THREADS 50
#define  MAX_NUM_MSGS    100
#define  MAX_EPIDS (MAX_NUM_LEVELS * MAX_NUM_KIDS * MAX_NUM_THREADS)
dest_t             exer_pid_list[MAX_EPIDS];

//test defines:
#define TC_NUM_RX_PTHREADS   100
#define TC_NUM_TX_PTHREADS   100
#define TC_NUM_CHILDREN      100
#define TC_NUM_MSGS          200
//Make tc_rx_pids global so the threads can add their pid to it.
dest_t             tc_rx_pids[TC_NUM_RX_PTHREADS];
dest_t             child_pids[TC_NUM_CHILDREN];
pthread_barrier_t  thread_barrier_1;
pthread_barrier_t  thread_barrier_2;

//---------------------------------------------------------------------------
// DoFork
//---------------------------------------------------------------------------
pid_t DoFork() {
pid_t child;

  child = fork();
  if (child < 0) {
    Err("Child failed to fork, %d \n", child);
  } 
  else if (child == 0) {
    //Child code: 
    //We need to (re)init the print lock, because the parent's 
    //copy maybe locked by a pthread that was mid-print. We are no
    //longer the parent copy, so the lock would be locked forever.
    if (pthread_mutex_init(&print_lock, NULL)) {
       printf("(child) pthread_mutex_init of print_lock failed\n");
       exit(EXIT_FAILURE);
    }
    srand(time(NULL)); //seed it
  }

  return child;
}

//---------------------------------------------------------------------------
// WaitForChildrenToEnd
// Wait for child process(es) to finish.
//---------------------------------------------------------------------------
void WaitForChildrenToEnd(int num_kids, unsigned int level) {
int   i;
int   wait_status;
int   cnt;
pid_t child_pid;
int   wait_options;

  TSLog("waiting for kids level %d\n", level);
  i   = 0;
  cnt = 0;
  wait_options = 0;
  while (i < num_kids) {
    child_pid = waitpid(-1, &wait_status, wait_options);
    if (child_pid == -1) {
      Err("from parent, wait returned an error, child died with an error? level %d",
		      level);
    }
    else {
      //child ended
      i++;
      int j;
      for (j = 0; j < num_kids; j++) {
	if (child_pids[j].pid == child_pid) {
	  child_pids[j].pid = 0;
	   break;
	}
      }
      //catch child errors and report them:
      if ((WIFEXITED(wait_status) == 0) ||
	  (WEXITSTATUS(wait_status) != EXIT_SUCCESS)) {
	      Err("child %d exited with status %d, level %d",
			      child_pid, WEXITSTATUS(wait_status), level);
      }
    }
  }

  TSLog("kids completed\n");
}

//---------------------------------------------------------------------------
// WaitForThreadsToEnd
// Wait for pthread(s) to finish.
//---------------------------------------------------------------------------
void WaitForThreadsToEnd(int num_threads, pthread_t *threads) {
int  i;

  i = 0;
  while (i < num_threads) {
    if (pthread_join(threads[i], NULL) != 0) {
      Err("from parent, join returned an error, thread died with an error?");
    }
    i++; 
    //TSLog("thread %d ended\n", i);
  }
}

//---------------------------------------------------------------------------
// GetRandDestIdx
// pids is an array indexed from 0-num_pids of possible dest-pids.
// Find a random, valid pid in this array and return its index.
// A pid is valid if it is non-zero. 
// A return value of NO_PIDS means we didn't find any pids left. 
//---------------------------------------------------------------------------
void GetRandDestIdx(dest_t *list, int num_pids, unsigned int *idx, pid_t *pid) {
unsigned int   cnt;

  if (num_pids == 0) {
    *idx  = NULL_IDX;
    *pid = NULL_PID; 
    return;
  }

  cnt = 0;
  *idx = (rand() % num_pids); //start someplace random

  do {
    //True for when threads are running:
    //Order is important here, someone else could have changed list[idx].
    //Use the idx to get the pid, then check the pid. If it's non-zero, we
    //can use it. It may get changed to zero (invalid) after we get it,
    //and this is ok b/c the send will return invalid or stopped.
    *idx = *idx + 1;
    if (*idx >= num_pids) *idx = 0; //wrap
    *pid = list[*idx].pid;
    if (*pid != 0) return; //done, return gd pid and gd idx
    cnt++;
  }
  while (cnt <= num_pids);

  *idx = NULL_IDX; 
  *pid = NULL_PID; 
}

//---------------------------------------------------------------------------
// DecMsgOpen
// Decrement the msg open cnt for this mailbox.
//---------------------------------------------------------------------------
void DecMsgOpen(pid_t pid, tc_args_t *args, char *prefix) {
unsigned int j;

  for (j = 0; j < args->num_dests; j++) {
    pthread_mutex_lock(&args->dest_lock);
    if (args->dest[j].pid == pid) {
      if (args->dest[j].msg_open == 0) {
        pthread_mutex_unlock(&args->dest_lock);
        Err("%s DecMsgOpen, msg_open already at zero for pid %d", 
			prefix, pid);
      }
      args->dest[j].msg_open--;
      pthread_mutex_unlock(&args->dest_lock);
      return; 
    }
    pthread_mutex_unlock(&args->dest_lock);
  }
  Err("%s DecMsgOpen, failed to find pid %d", prefix, pid);
}

//---------------------------------------------------------------------------
// DoManage
//---------------------------------------------------------------------------
void DoManage(bool stop, unsigned int *num_msgs) {
err_t err;
int my_num_msgs;

  err.act_code = ManageMailbox(stop, &my_num_msgs);
  *num_msgs = (unsigned int)my_num_msgs; 
  if (err.act_code != MAILBOX_GOOD) {
    Err("ManageMailbox failed with %s\n", 
		    ErrDecoder(err.act_code, err.act_msg));
  }
}

//---------------------------------------------------------------------------
// CheckNumMsgs
//---------------------------------------------------------------------------
void CheckNumMsgs(int exp_num) {
unsigned int num_msgs;

  DoManage(false, &num_msgs);
  if (num_msgs != exp_num) {
    Err("expected num_msgs %d, got %d", exp_num, num_msgs);
  }
}

//---------------------------------------------------------------------------
// RdOneMsg
// Note the special cases to force errors:
// ERR_BAD_MSGBUF: set '&msg' to null when it calls Rcvsg, to force a
//                 MSG_ARG_ERROR.
// ERR_BAD_LENPTR: set '&len' to 0 when it calls RcvMsg, to force a
//                 MSG_ARG_ERROR.
// ERR_BAD_DESTPTR: set '&dest' to 0 when it calls RcvMsg, to force a
//                 MSG_ARG_ERROR.
//---------------------------------------------------------------------------
int RdOneMsg(msg_t *msg, int exp_err, char *prefix) {
err_t err;
char  *p_msg;
int   *p_len;
int   *p_dest;
pid_t my_pid;

  my_pid = syscall(224); //gettid call, valid for threads too

  p_msg  = &msg->buf[0];
  p_len  = &msg->len;
  p_dest = &msg->pid;

  if (exp_err == ERR_BAD_MSGBUF) {
    //special case: force msg buf ptr to 0
    p_msg = 0;
    exp_err = MSG_ARG_ERROR;
  }
  else if (exp_err == ERR_BAD_LENPTR) {
    //special case: force len ptr to 0
    p_len = 0;
    exp_err = MSG_ARG_ERROR;
  }
  else if (exp_err == ERR_BAD_DESTPTR) {
    //special case: force dest ptr to 0
    p_dest = 0;
    exp_err = MSG_ARG_ERROR;
  }
  
  err.act_code = RcvMsg(p_dest, p_msg, p_len, msg->block);
     
  if (exp_err != ERR_NO_CHECK) {
    if (err.act_code != exp_err) {
      Err("(%d) %s, failed on RcvMsg, exp_err %s, got err %s", 
            my_pid, prefix,
            ErrDecoder(exp_err, err.exp_msg),
            ErrDecoder(err.act_code, err.act_msg));
    }
    else if (err.act_code != MAILBOX_GOOD) {
      //We got the expected err, but only print it if it
      //isn't good, b/c we'll get lots of gd msgs.
      TSLog("%sRcvMsg, expected and got err %s\n", 
            prefix,
            ErrDecoder(err.act_code, err.act_msg));
    }
  }
    
  if (err.act_code == MAILBOX_GOOD) { //good msg:
    if (msg->len == 0) {
      TSLog("%spid %d rcv'ed 0 length msg \n", prefix, my_pid);
      }
      else {
      TSLog("%spid %d rcv'ed msg from %d\n%s\n", 
		      prefix, my_pid, msg->pid, msg->buf);
      }
  }
    
  if ((exp_err != ERR_NO_CHECK) && (err.act_code == MAILBOX_GOOD) &&
    //need to include \0 in strlen, hence the +1
    (msg->is_string && ((strlen(msg->buf) + 1) != msg->len))) {
    Err("%sRcvMsg returned incorrect length, expected %d, got %d, msg\n%s\n",
        prefix, (strlen(msg->buf)+1), msg->len, msg->buf);
  }
    
  return err.act_code;
}

//---------------------------------------------------------------------------
// RdGdMsgsAndExitAfterXMsgs
//---------------------------------------------------------------------------
void RdGdMsgsAndExitAfterXMsgs(unsigned int num_msgs, bool msg_is_str, 
		               bool stop_first) {
msg_t  msg;
pid_t  tid;

  tid = syscall(224); //gettid call, valid for threads too
    
  msg.block = true;
  msg.is_string = msg_is_str;
  while (num_msgs > 0) {
    (void)RdOneMsg(&msg, MAILBOX_GOOD, "");
     num_msgs--;
  }

  //if stop_first is set, then stop our mailbox then exit:
  if (stop_first) DoManage(true, &num_msgs);

  //TSLog("testing done, rx pid %d\n", tid);
  exit(EXIT_SUCCESS);
}

//---------------------------------------------------------------------------
// RdGdMsgsAndExitAfterStop
// Works for parent/child/thread.
// Receive msgs until mailbox is stopped.
// Stopped is indicated by a MAILBOX_STOPPED return, or by the global stop 
// flag == true. We need the flag in addition to the MAILBOX_STOPPED because
// the mailbox could be stopped while we are outside of the RcvMsg function.
//---------------------------------------------------------------------------
void RdGdMsgsAndExitAfterStop(bool msg_is_str, bool is_thread) {
msg_t  msg;
pid_t  tid;
err_t  err;

  tid = syscall(224); //gettid call, valid for threads too

  //TSLog("testing starting, rx pid %d\n", tid);
  msg.block = true;
  msg.is_string = msg_is_str;
  do {
    err.act_code = RdOneMsg(&msg, ERR_NO_CHECK, "");
    if ((err.act_code != MAILBOX_GOOD) &&
        (err.act_code != MAILBOX_STOPPED)) {
      Err("tid %dRdGdMsgsAndExitAfterStop got bad rcv code, %s",
          tid, ErrDecoder(err.act_code, err.act_msg)); 
    }
  }
  while ((err.act_code != MAILBOX_STOPPED) && !this_mbox_stopped);

  //TSLog("testing done, rx pid %d\n", tid);
  if (is_thread) pthread_exit(0);
  else           exit(EXIT_SUCCESS);
}

//---------------------------------------------------------------------------
// SendOneMsg
// ERR_BAD_MSGBUF: set 'msg' to null when it calls Sendmsg, to force a
//                 MSG_ARG_ERROR.
//---------------------------------------------------------------------------
int SendOneMsg(msg_t *msg, int exp_err) {
err_t err;
char  *p_msg;
pid_t my_pid;

  my_pid = syscall(224);

  p_msg = msg->buf;

  if (exp_err == ERR_BAD_MSGBUF) {
    //special case: force msg buf ptr to zero 
    p_msg = 0;
    msg->len = (MAX_MSG_SIZE - 1); //any legal length
    exp_err = MSG_ARG_ERROR;
  }
    
  err.act_code = SendMsg(msg->pid, p_msg, msg->len, msg->block);
    
  if (exp_err != ERR_NO_CHECK) {
    if (err.act_code != exp_err) {
      if (msg->is_string) {
        TSLog("about to error, msg is %s", msg->buf);
      }
      Err("(%d) failed on SendMsg to %d, exp_err %s, got err %s", 
          my_pid, msg->pid, 
          ErrDecoder(exp_err, err.exp_msg),
          ErrDecoder(err.act_code, err.act_msg));
    }
    else if (err.act_code != MAILBOX_GOOD) {
      //We got the expected err, but only print the error-return if it
      //isn't good, b/c we'll send lots of gd msgs.
      TSLog("SendMsg, expected and got err %s \n", 
             ErrDecoder(err.act_code, err.act_msg));
    }
  }
    
  return err.act_code;
}

//---------------------------------------------------------------------------
// SendCharMsg
//---------------------------------------------------------------------------
int SendCharMsg(pid_t pid, bool block, char *str, int exp_err) {
msg_t msg;

  InitFullCharMsg(&msg, pid, block, str);
  return SendOneMsg(&msg, exp_err);
}

//---------------------------------------------------------------------------
// InitFullCharMsg
//---------------------------------------------------------------------------
void InitFullCharMsg(msg_t *msg, pid_t pid, bool block, char *str) {
  strncpy(msg->buf, str, MAX_MSG_SIZE);
  InitCharMsg(msg, pid, block);
}

//---------------------------------------------------------------------------
// InitCharMsg
// Call this AFTER msg->buf has been set to a string.
//---------------------------------------------------------------------------
void InitCharMsg(msg_t *msg, pid_t pid, bool block) {
  msg->is_string = true;
  msg->pid       = pid;
  msg->block     = block;
  msg->len       = (strlen(msg->buf) + 1); //include \0 also 
}

//---------------------------------------------------------------------------
// SendReqAckMsg
// Return true if msgs sent, return false otherwise.
//---------------------------------------------------------------------------
bool SendReqAck(pid_t me, unsigned int dest_idx, tc_args_t *args) {
msg_t  msg;
err_t  err;
pid_t  dest;

  //We have a dest_idx that was good at the time it was assigned to us.
  //Lock the dest_lock and check if dest_idx is still good. It could
  //have been set to zero by some other thread between the time that
  //idx was assigned and now.
  //If dest_idx still points to a valid pid, up its ref_cnt and save
  //the pid value, because once we unlock it, it can change.
  pthread_mutex_lock(&args->dest_lock);

  dest = args->dest[dest_idx].pid;
  if (dest == 0) {
    pthread_mutex_unlock(&args->dest_lock);
    return false;
  }

  snprintf(msg.buf, MAX_MSG_SIZE, "%s tx: %d  rx: %d", 
           REQ_ACK, me, dest);
  InitCharMsg(&msg, dest, true);

  //Increment use count, release lock.
  args->dest[dest_idx].msg_open++;
  pthread_mutex_unlock(&args->dest_lock);

  //Send msg:
  err.act_code = SendOneMsg(&msg, ERR_NO_CHECK);

  pthread_mutex_lock(&args->dest_lock);

  if ((err.act_code == MAILBOX_INVALID) ||
      (err.act_code == MAILBOX_STOPPED)) {
    //Only mark it invalid if it is not in use by someone else.
    //If it is in-use, then we'll mark it invalid when we next try 
    //to send to it and it isn't in-use. 
    args->dest[dest_idx].msg_open--;
    if (args->dest[dest_idx].msg_open == 0) {
      TSLog("%d retiring pid idx %d, %d\n", me, dest_idx, dest);
      args->dest[dest_idx].pid  = 0;
    }
    pthread_mutex_unlock(&args->dest_lock);
    return false;
  }
  else if (err.act_code != MAILBOX_GOOD) {
    pthread_mutex_unlock(&args->dest_lock);
    Err("(%d) failed on SendReqAck from %d to %d, got err %s", 
        me, dest, ErrDecoder(err.act_code, err.act_msg));
    return false;
  }

  pthread_mutex_unlock(&args->dest_lock);
  return true;
}

//---------------------------------------------------------------------------
// SendAckMsg
// Send ack msg. 
//---------------------------------------------------------------------------
void SendAck(pid_t me, pid_t ack_pid, tc_args_t *args) {
msg_t  msg;
    snprintf(msg.buf, MAX_MSG_SIZE, "%s tx: %d  rx: %d", ACK, me, ack_pid);
    InitCharMsg(&msg, ack_pid, true);
    (void)SendOneMsg(&msg, MAILBOX_GOOD);
}
      

//---------------------------------------------------------------------------
// SendMsgsThenExitAfterStop
// Send messages to dest until a non-good result. If this result is not
// stopped, report an error, otherwise exit.
// This can be used to send msgs until the mbox is full. 
//---------------------------------------------------------------------------
void SendMsgsThenExitAfterStop(pid_t dest) {
err_t err;

  do {
    //this will eventually blk when the mbox goes full:
    err.act_code = SendCharMsg(dest, true, "message", ERR_NO_CHECK);
    if ((err.act_code != MAILBOX_STOPPED) && (err.act_code != MAILBOX_GOOD)) {
      Err("failed on SendMsg to %d, exp_err GOOD/STOPPED, got err %s", 
          dest, ErrDecoder(err.act_code, err.act_msg));
    }
  }
  while (err.act_code == MAILBOX_GOOD);

  if (err.act_code != MAILBOX_STOPPED) {
	Err("failed on SendMsg, exp_err GOOD/STOPPED, got err %s", 
				ErrDecoder(err.act_code, err.act_msg));
  }
  exit(EXIT_SUCCESS);
}

//---------------------------------------------------------------------------
// SendXMsgsThenExit
//---------------------------------------------------------------------------
void *SendXMsgsThenExit( void *v_args) {
unsigned int   idx;
int            i;
msg_t          msg;
pid_t          tid;
pid_t          dest_pid;
tc_args_t      *args;

  args = (tc_args_t *)v_args;

  tid = syscall(224); //gettid call, valid for threads too

  if (args->im_a_child) {
    msg_t start_msg;

    TSLog(" \t\t\t\t\t\tstarting child %d\n", tid);

    //tell parent that we've started:
    TSLog("child %d to parent %d start msg\n", tid, args->master_pid);
    snprintf(msg.buf, MAX_MSG_SIZE, "start-msg  to parent");
    (void)SendCharMsg(args->master_pid, true, msg.buf, MAILBOX_GOOD);

    //sync up: wait for parent msg before starting
    TSLog("child %d waiting for parent %d start msg\n", tid, args->master_pid);
    start_msg.block = true;
    (void)RdOneMsg(&start_msg, MAILBOX_GOOD, "");
  }
  else TSLog(" \t\t\t\t\t\tstarting thread %d\n", tid);

  for (i = 0; i < args->num_msgs; i++) {

    GetRandDestIdx(args->dest, args->num_dests, &idx, &dest_pid);
    if (idx == NULL_IDX) {
	    Err("failed in SendXMsgsThenExit (child %d), no valid pids %d",
			    args->im_a_child, idx);
    }

    snprintf(msg.buf, MAX_MSG_SIZE, "SendXMsgs: sender %d -> rcv %d msg #%d", 
			  tid, dest_pid, i);
    InitCharMsg(&msg, dest_pid, true);

    //this will blk if the mbox goes full:
    (void)SendOneMsg(&msg, MAILBOX_GOOD);
  }


  if (args->im_a_child) exit(EXIT_SUCCESS);
  else  /*thread*/      pthread_exit(0);
}

//---------------------------------------------------------------------------
// TestGdMsgs
// Start a child. Parent and child each send/rcv 'max_msgs' msgs to each other.
//---------------------------------------------------------------------------
void TestGdMsgs() {
pid_t  master_pid;
pid_t  child_pid;
int    max_msgs = 25;

  TSLog("TEST: verify basic good msgs between child and parent\n");

  master_pid = getpid();

  //make the child:
  child_pid  = DoFork();

  if (max_msgs > NUM_MSGS_MBOX_FULL) {
	  //This test does X sends then X rcvs, blocking. It will hang if
	  //blocked at a full mailbox, so don't send too many msgs.
	  Err("This is a basic test and it will hang if max_msgs > NUM_MSGS_MBOX_FULL\n");
  }

  if (child_pid == 0) {
	msg_t  child_msg;
        int    msg_num;
	child_pid = syscall(224); //gettid call, valid for threads too

	for (msg_num=0; msg_num < max_msgs; msg_num++) {
	  snprintf(child_msg.buf, MAX_MSG_SIZE, 
			  "child %d -> parent %d msg #%d", 
			  child_pid, master_pid, msg_num);
	  InitCharMsg(&child_msg, master_pid, true);
	  (void)SendOneMsg(&child_msg, MAILBOX_GOOD);
	}

	for (msg_num=0; msg_num < max_msgs; msg_num++) {
	  child_msg.block = true;
	  (void)RdOneMsg(&child_msg, MAILBOX_GOOD, "(child) ");
	}
	exit(EXIT_SUCCESS);
  }

  //parent:
  msg_t  msg;
  int    i;
  for (i=0; i < max_msgs; i++) {
  	snprintf(msg.buf, MAX_MSG_SIZE, "parent %d -> child %d msg #%d", 
  			master_pid, child_pid, i);
	InitCharMsg(&msg, child_pid, true);
  	(void)SendOneMsg(&msg, MAILBOX_GOOD);
  }

  for (i=0; i < max_msgs; i++) {
	msg.block = true;
  	(void)RdOneMsg(&msg, MAILBOX_GOOD, "(parent) ");
  }

  WaitForChildrenToEnd(1, 0);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestFullWithChild
// Start a child. 
// The child will fill the parent's mailbox and block on the next send op.
// The parent will wait for a full mailbox then read one message.
// The child's send will unblock, then the child do a send send-not-block
// and expect MAILBOX_FULL.
//---------------------------------------------------------------------------
void TestFullWithChild() {
pid_t         master_pid;
pid_t         child_pid;
msg_t         msg;
unsigned int  num_msgs;

  TSLog("TEST: verify send-full with child \n");

  master_pid = getpid();

  //make the child:
  child_pid  = DoFork();

  if (child_pid == 0) {
	//child:
	msg_t  child_msg;
        int    msg_num;
	child_pid = syscall(224); //gettid call, valid for threads too

	for (msg_num=0; msg_num < NUM_MSGS_MBOX_FULL; msg_num++) {
    	  snprintf(child_msg.buf, MAX_MSG_SIZE, "child->parent msg %d", 
			  msg_num);
    	  (void)SendCharMsg(master_pid, true, child_msg.buf, MAILBOX_GOOD);
	}

	//Send one more msg, this will block/full.
	//--> parent will read one msg to unblock this
    	snprintf(child_msg.buf, MAX_MSG_SIZE, "child->parent msg1 (full)");
    	(void)SendCharMsg(master_pid, true, child_msg.buf, MAILBOX_GOOD);

	//Send one more msg, this time no-block. Should get full:
    	snprintf(child_msg.buf, MAX_MSG_SIZE, "child->parent msg2 (full)");
    	(void)SendCharMsg(master_pid, false, child_msg.buf, MAILBOX_FULL);

	exit(EXIT_SUCCESS);
  }

  //parent:
  //wait for full mailbox
  do {
    DoManage(false, &num_msgs);
  } while (num_msgs < NUM_MSGS_MBOX_FULL);

  TSLog("Mailbox full \n");

  //Unblock child:
  (void)RdOneMsg(&msg, MAILBOX_GOOD, "");

  //Wait for child to end:
  WaitForChildrenToEnd(1, 0);

  //exit with a full, not-stopped mailbox

  TSLog(TEST_PASSED);
}


//---------------------------------------------------------------------------
// TestZeroLenMsg
// Send a zero length msg to ourself, then receive it.
//---------------------------------------------------------------------------
void TestZeroLenMsg() {
pid_t  master_pid;
pid_t  child_pid;
msg_t  msg;

  TSLog("TEST: verify zero-length msg works \n");

  master_pid = getpid();

  //make the child:
  child_pid  = DoFork();

  if (child_pid == 0) {
	  RdGdMsgsAndExitAfterXMsgs(1, false, false);
  }

  //parent:
  InitFullCharMsg(&msg, child_pid, true, ""); 
  msg.len = 0; //set msg length to zero
  (void)SendOneMsg(&msg, MAILBOX_GOOD);
  TSLog("Parent sent zero length msg\n");

  WaitForChildrenToEnd(1, 0);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestBadSendArg
// Start a child, send a good msg to the child, then a msg with a bad (null)
// msg source, then a send good msg.
//---------------------------------------------------------------------------
void TestBadSendArg() {
pid_t  master_pid;
pid_t  child_pid;

  TSLog("TEST: verify bad-send-arg (src msg ptr is 0) \n");

  master_pid = getpid();

  //make the child:
  child_pid  = DoFork();

  if (child_pid == 0) {
	  RdGdMsgsAndExitAfterXMsgs(2, true, false);
  }

  //parent:
  (void)SendCharMsg(child_pid, true, "msg #1", MAILBOX_GOOD);
  (void)SendCharMsg(child_pid, true, "msg #2 - bad msg buf ptr", ERR_BAD_MSGBUF);
  (void)SendCharMsg(child_pid, true, "msg #3", MAILBOX_GOOD);

  WaitForChildrenToEnd(1, 0);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestSendPid1
//---------------------------------------------------------------------------
void TestSendPid1() {

  TSLog("TEST: verify send-to-pid-1 (expect mailbox-invalid) \n");

  (void)SendCharMsg(1, true, "better not get sent", MAILBOX_INVALID);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestRcvBlkStoppedEmMbox
// Stop our empty mailbox. RcvMsg()-blocked the mailbox, it should
// return empty even though the RcvMsg was blockinge This verifies that 
// a read-block of a stopped mailbox does not block (because if it blocked, 
// it would block forever, as no new msgs can ever arrive).
//---------------------------------------------------------------------------
void TestRcvBlkStoppedEmMbox() {
unsigned int num_msgs;
msg_t        msg;

  TSLog("TEST: verify RcvMsg-blocking of a stopped and empty mailbox\n");

  TSLog("stop the empty mailbox\n");
  DoManage(true, &num_msgs);

  msg.block = true;
  RdOneMsg(&msg, MAILBOX_EMPTY, "");
  TSLog("RcvMsg-blking from a stopped empty worked\n");

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestFullEmpty
// Verify empty mailbox, then full mailbox, then empty. Then refill the
// mailbox and end with a full mailbox, so exit has stop the mailbox and purge
// the leftover messages.
//---------------------------------------------------------------------------
void TestFullEmpty() {
pid_t  master_pid;
msg_t  msg;
int    i;

  TSLog("TEST: verify send-full, receive-empty \n");

  master_pid = getpid();

  //read, should get empty:
  msg.block = false;
  (void)RdOneMsg(&msg, MAILBOX_EMPTY, "");

  //fill mailbox:
  for (i = 0; i < NUM_MSGS_MBOX_FULL; i++) {
    snprintf(msg.buf, MAX_MSG_SIZE, "self-msg #%d",  i);
    (void)SendCharMsg(master_pid, true, msg.buf, MAILBOX_GOOD);
    CheckNumMsgs(i+1);
  }

  //This msg won't get sent, mbox is full. Non-blocking.
  snprintf(msg.buf, MAX_MSG_SIZE, "self-msg #%d",  i);
  (void)SendCharMsg(master_pid, false, msg.buf, MAILBOX_FULL);
  CheckNumMsgs(NUM_MSGS_MBOX_FULL);
  TSLog("Filled mbox %d\n", i);

  msg.block = false; 
  for (i = 0; i < NUM_MSGS_MBOX_FULL; i++) {
    //read msgs, they should read cleanly:
    (void)RdOneMsg(&msg, MAILBOX_GOOD, "");
  }

  //one more read, should get empty:
  msg.block = false;
  (void)RdOneMsg(&msg, MAILBOX_EMPTY, "");

  //fill mailbox again:
  InitFullCharMsg(&msg, master_pid, true, "");
  for (i = 0; i < NUM_MSGS_MBOX_FULL; i++) {
    snprintf(msg.buf, MAX_MSG_SIZE, "gets deleted by exit #%d",  i);
    (void)SendCharMsg(master_pid, true, msg.buf, MAILBOX_GOOD);
    CheckNumMsgs(i+1);
  }

  //exit with a full, not-stopped mailbox

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestBadRcvArg
// Start a child, send it three msgs.
// Child reads the first msg, the attempts to read the second msg with a bad
// msg buffer, then reads the third msg correctly.
// Repeat for a bad length ptr.
// Repeat for a bad dest-pid ptr.
//---------------------------------------------------------------------------
void TestBadRcvArg() {
pid_t  child_pid;

  TSLog("TEST: verify bad-rcv-arg (rcv msg ptr is 0) \n");

  //make the child:
  child_pid  = DoFork();

  if (child_pid == 0) {
    msg_t child_msg;

    child_msg.block = true;

    (void)RdOneMsg(&child_msg, MAILBOX_GOOD, "");
    //bad msg ptr:
    (void)RdOneMsg(&child_msg, ERR_BAD_MSGBUF, "");
    (void)RdOneMsg(&child_msg, MAILBOX_GOOD, "");
    //bad len ptr:
    (void)RdOneMsg(&child_msg, ERR_BAD_LENPTR, "");
    (void)RdOneMsg(&child_msg, MAILBOX_GOOD, "");
    //bad dest ptr:
    (void)RdOneMsg(&child_msg, ERR_BAD_DESTPTR, "");
    (void)RdOneMsg(&child_msg, MAILBOX_GOOD, "");

    exit(EXIT_SUCCESS);
  }

  //parent:
  //send six msgs, then wait for child to end
  (void)SendCharMsg(child_pid, true, "message #1", MAILBOX_GOOD);
  (void)SendCharMsg(child_pid, true, "message #2 -- rcv drops with arg error", 
		    MAILBOX_GOOD);
  (void)SendCharMsg(child_pid, true, "message #3", MAILBOX_GOOD);

  (void)SendCharMsg(child_pid, true, "message #4 -- rcv drops with arg error", 
		    MAILBOX_GOOD);
  (void)SendCharMsg(child_pid, true, "message #5", MAILBOX_GOOD);

  (void)SendCharMsg(child_pid, true, "message #6 -- rcv drops with arg error", 
		    MAILBOX_GOOD);
  (void)SendCharMsg(child_pid, true, "message #7", MAILBOX_GOOD);

  WaitForChildrenToEnd(1, 0);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TestBadManageArg
//---------------------------------------------------------------------------
void TestBadManageArg() {
int    *p_num_msgs;
err_t  err;

  TSLog("TEST: verify bad-manage-arg (num_msgs ptr is 0) \n");

  p_num_msgs = 0;

  err.act_code = ManageMailbox(false, p_num_msgs);
  if (err.act_code != MSG_ARG_ERROR) {
      Err("ManageMailbox failed with %s\n", 
		      ErrDecoder(err.act_code, err.act_msg));
  }
  TSLog("ManageMailbox correctly got MSG_ARG_ERROR \n");

  CheckNumMsgs(0);

  TSLog(TEST_PASSED);
}


//---------------------------------------------------------------------------
// GetAcrPid
//---------------------------------------------------------------------------
pid_t GetAckPid(tc_args_t *args, char *msg_buf) {
char *ack_char;
pid_t ack_pid;
msg_t copy;

  //I think strtok is thread-safe, as it holds state. Lock it.
  pthread_mutex_lock(&args->dest_lock);
  strncpy(copy.buf, msg_buf, MAX_MSG_SIZE);
  //fifth arg is the pid to ack, use brute force (this is a hack, fix it):
  if ((ack_char = strtok(copy.buf, " ")) == NULL) Err("bad token");
  if ((ack_char = strtok(NULL, " ")) == NULL) Err("bad token");
  if ((ack_char = strtok(NULL, " ")) == NULL) Err("bad token");
  if ((ack_char = strtok(NULL, " ")) == NULL) Err("bad token");
  if ((ack_char = strtok(NULL, " ")) == NULL) Err("bad token");
  ack_pid = atoi(ack_char);
  pthread_mutex_unlock(&args->dest_lock);
  return ack_pid;
}

//---------------------------------------------------------------------------
// RdUntilAck
// Read msgs until the msg_open cnt for our ack_idx is zero. 
// If 'wait_for_empty' is set, then read msgs until the mailbox is empty.
// If 'done_at_stop' is set, then read msgs until the args->stop flag 
// indicates we've stopped. 
//
// This is run by threads (test) and children (end of test cleanup).
//---------------------------------------------------------------------------
void RdUntilAck(pid_t me, unsigned int ack_idx, tc_args_t *args, 
		bool wait_for_empty, bool done_at_stop) {
bool         done;
msg_t        rd_msg;
err_t        err;
char         info_msg[256];
pid_t        ack_pid;
unsigned int cnt;

  //Wait for the msg, non-blocking. Since we have threads active, this
  //process may not get the acknowledge (another thread may get it).
  //Read msgs while waiting for the ack-msg.
  done = false; 
  cnt  = 0;
  do {
    rd_msg.block = false;
    err.act_code = RdOneMsg(&rd_msg, ERR_NO_CHECK, "");
    if ((err.act_code != MAILBOX_GOOD) && (err.act_code != MAILBOX_EMPTY)) {
      Err("(%d) failed on RcvMsg %d, got err %s", 
          me, ErrDecoder(err.act_code, err.act_msg));
    }
    else if (wait_for_empty && (err.act_code == MAILBOX_EMPTY)) return;
    else if (err.act_code == MAILBOX_EMPTY) {
      usleep(rand() % 15);
    }
    else { //we got a good msg
 
      TSLog("msg, my-pid %d msg, %s, from %d, level %d\n",
	    me, rd_msg.buf, rd_msg.pid, args->level);
      //If msg is a req_ack, ack it.
      if (strncmp(rd_msg.buf, REQ_ACK, strlen(REQ_ACK)) == 0) {
        //Msg is a request-for-ack. Ack it.
	ack_pid = GetAckPid(args, rd_msg.buf);
        SendAck(me, ack_pid, args);
      }
      else if ((strncmp(rd_msg.buf, ACK, strlen(ACK)) == 0)) {
 	//Msg is an ack, record it. 
	ack_pid = GetAckPid(args, rd_msg.buf);
        snprintf(info_msg, 256,
			  "pid %d, msg ___%s___, from %d, ack_id %d level %d\n",
			  me, rd_msg.buf, rd_msg.pid, ack_pid, args->level);
        DecMsgOpen(ack_pid, args, info_msg); 
      }
      else {
	Err("pid %d RdUntil read bad msg, %s, from %d, level %d",
	    me, rd_msg.buf, rd_msg.pid, args->level);
      }
    }

    //If msg_open is now zero, then we got our ack (or someone else got it
    //for us). 
    //If we're in done_at_stop mode, wait for all threads to send before
    //we exit.
    pthread_mutex_lock(&args->dest_lock);
    if (done_at_stop) {
      done = (args->stop_exer && (args->num_threads_running == 0));
    }
    else done = (args->dest[ack_idx].msg_open == 0);
    pthread_mutex_unlock(&args->dest_lock);

    cnt++;
    if (cnt > 1000000) {
      //timeout just in case...
      if (done_at_stop) {
        Err("RdUntil timeout waiting for stop, tid %d, level %d", 
			    me, args->level);
      }
      else {
	Err("RdUntil timeout waiting for stop or ack from idx %d pid %d", 
	    ack_idx, args->dest[ack_idx].pid);
      }
    }
  }
  while (!done);
}


//---------------------------------------------------------------------------
// DoExer
// for num_msgs {
//   1. Select a random destination; 
//   1a. If no valid destinations, then done;
//   2. Send dest a msg;
//   2a. If send == invalid or stopped, go to 1;
//   3. Wait for ack msg from dest;
//   3a. If we get a new-msg, then send ack;
// }
// Either Manage-stop or exit.
//
// This is always called by a thread.
// 
//---------------------------------------------------------------------------
void *DoExer(void *arg1) {
pid_t        tid;
unsigned int idx;
unsigned int i;
ex_args_t    *ex_args;
tc_args_t    *args;
pid_t        dest_pid;

  tid = syscall(224); //gettid call, valid for threads too

  ex_args = (ex_args_t *)arg1;
  args    =  ex_args->tc_args;

  args->dest[ex_args->my_idx].pid = tid;

  TSLog("exer starting thread, pid %d level %d\n", tid, args->level);

  //let main process know we're ready:
  pthread_barrier_wait(&thread_barrier_1);

  //wait for main process to tell us that kids are ready too:
  pthread_barrier_wait(&thread_barrier_2);


  TSLog("exer post-sync thread, pid %d level %d\n", tid, args->level);

  for (i = 0; i < args->num_dests; i++) {
    TSLog("level %d, thread %d, dest pid %d\n",
	  args->level, tid, args->dest[i].pid);
  }

  i = 0;
  while (i < args->num_msgs) {
    //Get msg destination:
    GetRandDestIdx(args->dest, args->num_dests, &idx, &dest_pid);
    if (idx == NULL_IDX) {
      TSLog("Exer level %d: no valid pids left\n", args->level);
      pthread_exit(0);
    }
    else {
      //Send msg. If we didn't send it for some reason, then loop back up
      //and try to send another msg.
      if (!SendReqAck(tid, idx, args)) continue;
      i++;

      //Read msgs until our idx is ack'ed.
      RdUntilAck(tid, idx, args, false, false);
    }
  }

  TSLog("Exer level %d, pid %d done with send-req-acks \n", args->level, tid);

  //Indicate that we're done:
  pthread_mutex_lock(&args->dest_lock);
  args->num_threads_running--;
  pthread_mutex_unlock(&args->dest_lock);

  //Wait for all threads to be done before we exit. Another thread
  //or grandchild may be sending to our dest-pid, so we need to wait
  //to be told we can exit.  
  RdUntilAck(tid, idx, args, false, true);

  TSLog("Exer level %d, pid %d done \n", args->level, tid);

  pthread_exit(0);

}

//---------------------------------------------------------------------------
// DoRx
// Receive the specified number of msgs then exit.
//---------------------------------------------------------------------------
void DoRx(int my_num, unsigned int num_msgs, bool stop_first) {
pid_t tid;

	tid = syscall(224); //gettid call, valid for threads too
	TSLog("starting rx pid %d, num_msgs %d\n", tid, num_msgs);
	RdGdMsgsAndExitAfterXMsgs(num_msgs, true, stop_first);
}

//---------------------------------------------------------------------------
// DoTx
//---------------------------------------------------------------------------
void DoTx(int my_num, int num_pids, dest_t *rx_pids, int max_num_msgs) {
pid_t         tid;
unsigned int  idx;
msg_t         msg;
err_t         err;
pid_t         dest_pid;

  tid = syscall(224); //gettid call, valid for threads too

  TSLog("starting tx pid %d\n", tid);

  while (1) {

    GetRandDestIdx(rx_pids, num_pids, &idx, &dest_pid);

    if (idx == NULL_IDX) {
      //Send msgs until there are no rxs left:
      //TSLog("DoTx: testing done, tx pid %d\n", tid);
      exit(EXIT_SUCCESS);
    }

    snprintf(msg.buf, MAX_MSG_SIZE, "msg src: %d,  dest: %0d", tid, dest_pid);
    InitCharMsg(&msg, dest_pid, true); 
    err.act_code = SendMsg(msg.pid, msg.buf, msg.len, msg.block);

    if (err.act_code == MAILBOX_INVALID) {
      //retire the pid only when it is invalid. We'll continue to try to send
      //to stopped pids.
      //TSLog("retiring rx[%d] %d\n", idx, rx_pids[idx].pid);
      rx_pids[idx].pid = 0; //this rx is gone
    }
    else if ((err.act_code != MAILBOX_GOOD)  && 
	     (err.act_code != MAILBOX_STOPPED)) {
      Err("DoTx send error, err was %s",
	   ErrDecoder(err.act_code, err.act_msg)); 
    }
  }
	
}

//---------------------------------------------------------------------------
// TestDeletedMboxs
// This test verifies sending to mailboxes that are being destroyed.
//
// Start a large number of receiving children.
// Start a large number of sending children.
// The receivers will read some random number of messages then exit.
// The senders will send messsages to the receivers until the receivers
// mailboxes are gone. 
// The senders end when there are no more receivers to send to. 
//---------------------------------------------------------------------------
#define NUM_TX_PIDS        100
#define NUM_RX_PIDS        100
#define MAX_DELMBOX_MSGS   200
void TestDeletedMboxs(bool stop_first) {
dest_t  tx_pid[NUM_TX_PIDS];
dest_t  rx_pid[NUM_RX_PIDS];
int     i;

  if (stop_first) 
    TSLog("TEST: verify mailbox-stop-destroy while senders active \n"); 
  else
    TSLog("TEST: verify mailbox-destroy while senders active \n"); 

  //make rx pids first:
  for (i = 0; i < NUM_RX_PIDS; i++) {
    rx_pid[i].pid = DoFork();
    if (rx_pid[i].pid == 0) DoRx(i, (rand()%MAX_DELMBOX_MSGS), stop_first);
  }

  //Make tx pids. TX pids know RX pids because they have access to rx_pid[].
  for (i = 0; i < NUM_TX_PIDS; i++) {
    tx_pid[i].pid = DoFork();
    if (tx_pid[i].pid == 0) DoTx(i, NUM_RX_PIDS, &rx_pid[0], MAX_DELMBOX_MSGS);
  }

  //parent waits for all kids.
  WaitForChildrenToEnd((NUM_RX_PIDS + NUM_TX_PIDS), 0);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// TooLong
// Send ourself a mix of good and too-long messages:
// good
// too-long random/50x
// good
// too-long (max+1)
// good (but == max)
// too-long negative length (aka, really long)
// 
// When a msg with len == 500 is sent, it will get an  error (msg too long). 
// It will also put the kernel's mailboxes into slow-mode. In slow-mode, 
// the SendMsg kernel call will issue a sleep in a key window between when
// it has attained the mb pointer, but before it has used the pointer. This
// can be used to verify that the mailbox implementation does not have
// a race condition between sending and mailbox-stopping/destroy.
//---------------------------------------------------------------------------
void TooLong(too_long_mode_e mode) {
pid_t        master_pid;
msg_t        msg;
unsigned int cnt;
unsigned int i, j;

  TSLog("TEST: verify too-long msg\n");

  master_pid = getpid();
  cnt = 0;

  InitFullCharMsg(&msg, master_pid, true, ""); 
  if (mode == SLOW_MODE_ON)      msg.len = 500; //enables slow-mode 
  else if(mode == SLOW_MODE_OFF) msg.len = 501; //disables slow-mode 
  else {
	  //some random too-long length, but not 500/501:
	  msg.len = (MAX_MSG_SIZE + 1 + (rand() % 100));
  }

  //Send a good message first:
  TSLog("send good message\n");
  (void)SendCharMsg(master_pid, true, "good message1", MAILBOX_GOOD);
  cnt++;
  CheckNumMsgs(cnt); //check the cnt

  //Send full+10 too long messages. Send more too-long msgs than there is 
  //'good msg space', to verify that the too-long msgs don't use/reserve
  //any mailbox space.
  msg.block = true;
  TSLog("send too-long random/50x\n");
  for (i = 0; i < (NUM_MSGS_MBOX_FULL+10); i++) {
    (void)SendOneMsg(&msg, MSG_TOO_LONG);
    CheckNumMsgs(cnt); //check the cnt
  }

  //Another good message:
  TSLog("send good message\n");
  (void)SendCharMsg(master_pid, true, "good message2", MAILBOX_GOOD);
  cnt++;
  CheckNumMsgs(cnt); //check the cnt

  //Another too long message, exact max+1 this time:
  msg.len = (MAX_MSG_SIZE + 1);
  TSLog("send too-long max+1\n");
  (void)SendOneMsg(&msg, MSG_TOO_LONG);
  CheckNumMsgs(cnt); //check the cnt

  //Send a length==max message. This is a good message.
  for (i = 0, j=0; i < (MAX_MSG_SIZE-1); i++) {
    snprintf(&msg.buf[i], 2, "%0d", j); //includes the \0 also
    j = (j + 1) % 10; //just want one digit
  }
  //strlen+1 because strlen doesn't include the trailing \0.
  if ((strlen(msg.buf)+1) != MAX_MSG_SIZE) {
	  TSLog("Error coming, len %d, msg %s\n", 
			  strlen(msg.buf), msg.buf);
	  Err("messed up the code to make the exact length too-long msg");
  }
  else TSLog("message length is %d\n", (strlen(msg.buf)+1));
  msg.len = MAX_MSG_SIZE;
  TSLog("send good, length==max\n");
  (void)SendOneMsg(&msg, MAILBOX_GOOD);
  cnt++;
  CheckNumMsgs(cnt); //check the cnt

  //Another too long message, exact max+1 this time:
  msg.len = (MAX_MSG_SIZE + 1);
  TSLog("send too-long negative length\n");
  (void)SendOneMsg(&msg, MSG_TOO_LONG);
  CheckNumMsgs(cnt); //check the cnt

  //Another good message:
  TSLog("send good message\n");
  (void)SendCharMsg(master_pid, true, "good message4", MAILBOX_GOOD);
  cnt++;
  CheckNumMsgs(cnt); //check the cnt

  //read good messages:
  for (i = 0; i < cnt; i++) {
    msg.block = true;
    (void)RdOneMsg(&msg, MAILBOX_GOOD, "");
  }

  //don't block, this should get an empty error:
  msg.block = false; 
  (void)RdOneMsg(&msg, MAILBOX_EMPTY, "");

  if (mode == SLOW_MODE_ON) {
	  TSLog("\n\n-------> SendMsg SLOW MODE ENABLED <-------\n\n");
  }
  else if(mode == SLOW_MODE_OFF) {
	  TSLog("\n\n-------> SendMsg SLOW MODE DISABLED <-------\n\n");
  }

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// VerifyAllMsgLens
//---------------------------------------------------------------------------
void VerifyAllMsgLens() {
pid_t        master_pid;
msg_t        tx_msg;
msg_t        rx_msg;
unsigned int i, j;
int          test_len;

  TSLog("TEST: verify all message-lengths \n");

  master_pid = getpid();

  InitFullCharMsg(&tx_msg, master_pid, true, ""); 
  InitFullCharMsg(&rx_msg, master_pid, true, ""); 
   
  for (test_len = 0; test_len < MAX_MSG_SIZE; test_len++) {

    if (test_len > 1) {
      //Make the message. Max is len-1 because of trailing \0.
      for (i = 0, j=0; i < (test_len-1); i++) {
        snprintf(&tx_msg.buf[i], 2, "%0d", j); //includes the \0 also
        j = (j + 1) % 10; //just want one digit
      }
    }
    else if (test_len == 1) tx_msg.buf[0] = '\0';

    tx_msg.len = test_len; 
    if ((test_len != 0) && (tx_msg.len != (strlen(tx_msg.buf)+1))) {
	    Err("bad test code, msg len wrong %d", test_len);
    }
    TSLog("message length is %d \n", tx_msg.len);
    (void)SendOneMsg(&tx_msg, MAILBOX_GOOD);
    CheckNumMsgs(1); //check the cnt

    //read the msg, check it:
    rx_msg.is_string = (test_len != 0); 
    (void)RdOneMsg(&rx_msg, MAILBOX_GOOD, "");
    if (rx_msg.len != tx_msg.len) {
      Err("rx_msg.len %d != tx_msg.len %d", rx_msg.len, tx_msg.len);
    }
    if (rx_msg.len != 0) {
      if (strcmp(tx_msg.buf, rx_msg.buf) != 0) {
        Err("rx msg != tx_msg, rx_msg %d", rx_msg.len);
      }
    }
  }

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// ThreadSharesMbox
// Thread code for test TestThreadSharesMbox
//---------------------------------------------------------------------------
void *ThreadSharesMbox(void *arg) {
pid_t master_pid;
pid_t tid;
err_t err;
msg_t msg;
char  thread_msg[] = "thread_msg";
char  parent_msg[] = "parent_msg";

  tid = syscall(224);
  master_pid = (pid_t)arg;

  //send a msg to our tid, then to our parent:
  err.act_code = SendCharMsg(tid,        true, thread_msg, MAILBOX_GOOD);
  err.act_code = SendCharMsg(master_pid, true, parent_msg, MAILBOX_GOOD);

  //tell the parent that we sent the msgs:
  pthread_barrier_wait(&thread_barrier_1);

  CheckNumMsgs(2);

  //wait for the parent to verify num-msgs:
  pthread_barrier_wait(&thread_barrier_2);

  //read/verify the msgs:
  msg.block = false;
  (void)RdOneMsg(&msg, MAILBOX_GOOD, "(thread msg)");
  if (strcmp(msg.buf, thread_msg) != 0) {
	  Err("thread msg contents ==%s== did not match expected ==%s==",
			  msg.buf, thread_msg);
  }
  (void)RdOneMsg(&msg, MAILBOX_GOOD, "(thread msg)");
  if (strcmp(msg.buf, parent_msg) != 0) {
	  Err("parent msg contents ==%s== did not match expected ==%s==",
			  msg.buf, parent_msg);
  }

  pthread_exit(0);

}

//---------------------------------------------------------------------------
// TestThreadSharesMbox
// Start a thread. Send a msg from the parent to the thread. The parent's
// msg cnt should increment by one, proving that the thread uses the same
// mbox as the parent.
//---------------------------------------------------------------------------
void TestThreadSharesMbox() {
pid_t     master_pid;
pthread_t thread;

  TSLog("TEST: verify threads share parent's mailbox \n");

  if (pthread_barrier_init(&thread_barrier_1, NULL, 2)) {
    Err("pthread_barrier_init failed \n");
  }
  if (pthread_barrier_init(&thread_barrier_2, NULL, 2)) {
    Err("pthread_barrier_init failed \n");
  }

  master_pid = getpid();

  //verify empty mailbox:
  CheckNumMsgs(0);

  //start thread:
  pthread_create(&thread, NULL, ThreadSharesMbox, (void *)master_pid);

  //At this barrier, thread has sent itself one msg, and us (parent) one msg.
  //Thus, our mailbox count should be two.
  pthread_barrier_wait(&thread_barrier_1);
  CheckNumMsgs(2);
  pthread_barrier_wait(&thread_barrier_2);

  //Thread will read/verify both messages then exit.
  WaitForThreadsToEnd(1, &thread);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// MStopThread
// The pthreads execute this code then exit.
// Wait for the mbox to be full then call ManageMailbox(stop=true).
// Next, read a message. The read should return mailbox-good/empty. 
//---------------------------------------------------------------------------
void *MStopThread(void *arg) {
err_t          err;
pid_t          tid;
unsigned int   num_msgs;
msg_t          msg;

  tid = syscall(224); //gettid call, valid for threads too
  do {
    //non-blocking, we just want the mbox to fill up
    err.act_code = SendCharMsg(tid, false, "thread msg", ERR_NO_CHECK);
    if ((err.act_code != MAILBOX_FULL) && (err.act_code != MAILBOX_GOOD) &&
		    (err.act_code != MAILBOX_STOPPED)) {
      Err("thread failed on send-until-full with %s\n",
          ErrDecoder(err.act_code, err.act_msg));
    }
  }
  while ((err.act_code != MAILBOX_FULL) && (err.act_code != MAILBOX_STOPPED));
    
  //mailbox is full (or already stopped), issue stop
  DoManage(true, &num_msgs);  
  TSLog("Issued ManageMailbox(stop)\n");
  
  //Read the msg. This can return:
  //  stopping: if the stopper is still executing its Manage-stop;
  //  good: read a msg;
  //  empty: all msgs have been read.
  //We read the msg either blocking or non-blocking. 
  msg.block = (rand() & 1);
  err.act_code = RdOneMsg(&msg, ERR_NO_CHECK, "");
  if ((err.act_code != MAILBOX_EMPTY) && (err.act_code != MAILBOX_GOOD) &&
      (err.act_code != MAILBOX_STOPPED)) {
     Err("thread failed on read-after-stop with %s\n",
          ErrDecoder(err.act_code, err.act_msg));
  }

  //mailbox can still be stopping here (as opposed to stopped):
  err.act_code = RdOneMsg(&msg, ERR_NO_CHECK, "");
  if ((err.act_code != MAILBOX_EMPTY) && (err.act_code != MAILBOX_GOOD) &&
      (err.act_code != MAILBOX_STOPPED)) {
     Err("Post ManageMailbox-stopped, did RcvMsg, did not get good/empty, got %s",
		     ErrDecoder(err.act_code, err.act_msg));
  }
  
  //wait to finish:  
  pthread_barrier_wait(&thread_barrier_1);

  pthread_exit(0);
}

//---------------------------------------------------------------------------
// TestMultipleStops
// Start multiple pthreads.
// Start mulitple children.
// The children will send many msgs to this process's mailbox.
// Once the mailbox is full, the pthreads will issue ManageMailbox with
// stop set, and they will follow this with a RcvMsg. The first ManageMailbox
// will stop the mailbox, the others will wait/pend until the stopper is
// done, then they will return just as they would if the mailbox was already
// stopped.
// The ManageMailbox calls are followed by RcvMsg calls. The RcvMsg calls
// should not ever get MAILBOX_STOPPED.
//---------------------------------------------------------------------------
#define MSTOPS_NUM_CHILDREN 100
#define MSTOPS_NUM_PTHREADS 100
void TestMultipleStops() {
pid_t     master_pid;
pid_t     child_pid[MSTOPS_NUM_CHILDREN];
pthread_t thread[MSTOPS_NUM_PTHREADS];
int       i;

  TSLog("TEST: verify multiple ManageMailbox(stop)\n");

  master_pid = getpid();

  //include parent in this barrier also:
  if (pthread_barrier_init(&thread_barrier_1, NULL, 
		           (MSTOPS_NUM_PTHREADS+1))) {
    Err("pthread_barrier_init failed \n");
  }

  //start children:
  for (i = 0; i < MSTOPS_NUM_CHILDREN; i++) {
    child_pid[i] = DoFork();
    if (child_pid[i] == 0) (void)SendMsgsThenExitAfterStop(master_pid);
  }

  //start pthreads:
  for (i = 0; i < MSTOPS_NUM_PTHREADS; i++) {
    pthread_create(&thread[i], NULL, MStopThread, (void *)i);
  }

  //wait for kids and threads:
  WaitForChildrenToEnd(MSTOPS_NUM_CHILDREN, 0);

  //end pthreads once children have ended:
  pthread_barrier_wait(&thread_barrier_1);
  WaitForThreadsToEnd(MSTOPS_NUM_PTHREADS, thread);

  TSLog(TEST_PASSED);

}

//---------------------------------------------------------------------------
// RecordTidAndRdMsgs
//---------------------------------------------------------------------------
void *RecordTidAndRdMsgs(void *arg1) {
pid_t     tid;
int       my_idx;   

  my_idx = (int)arg1;

  //record my tid
  tid = syscall(224);
  tc_rx_pids[my_idx].pid = tid;

  //Tell our parent that we exist and that we've filled in tid
  pthread_barrier_wait(&thread_barrier_1);

  TSLog("rcv thread pid %d started\n", tid);

  //Don't start reading until our parent tells us to. Our parent
  //(and only our parent) needs to read msgs from the children so
  //it knows they're ready.
  pthread_barrier_wait(&thread_barrier_2);

  //wait for all rcvs to start:
  RdGdMsgsAndExitAfterStop(true, true);  

  return NULL; //fixed compiler error, but we can't get here
}

//---------------------------------------------------------------------------
// TestThreadChildrenMsgs
// Start threads and children, send messages between them.
// Threads and children are different, in that threads share a mailbox
// with the parent, and children do not.
//---------------------------------------------------------------------------
void TestThreadChildrenMsgs() {
pid_t         master_pid;
pthread_t     rx_threads[TC_NUM_RX_PTHREADS];
pthread_t     tx_threads[TC_NUM_TX_PTHREADS];
int           i; 
int           j; 
unsigned int  num_msgs;
tc_args_t args[TC_NUM_CHILDREN + TC_NUM_TX_PTHREADS];

  TSLog("TEST: verify gd msgs between threads and children. \n");

  master_pid = getpid();
  TSLog("master_pid %d \n", master_pid);

  //include parent in this barrier too (the '+ 1'):
  if (pthread_barrier_init(&thread_barrier_1, NULL, 
		           (TC_NUM_RX_PTHREADS+1))) {
    Err("pthread_barrier_init failed \n");
  }
  if (pthread_barrier_init(&thread_barrier_2, NULL, 
		           (TC_NUM_RX_PTHREADS+1))) {
    Err("pthread_barrier_init failed \n");
  }

  TSLog("start rcv pthreads \n");
  //start rx pthreads, before children, so children can send to the
  //rx-pids, not just the parent-pid:
  for (i = 0; i < TC_NUM_RX_PTHREADS; i++) {
    tc_rx_pids[i].pid = 0; //gets filled in by thread
    pthread_create(&rx_threads[i], NULL, RecordTidAndRdMsgs, (void *)(i));
  }

  //Wait for all rx threads to have been created, and for this parent thread
  //to know this has occurred.  This means that tx_rx_pids[] is filled in
  //by rx_threads and is good. 
  TSLog("syncing rx1\n");
  pthread_barrier_wait(&thread_barrier_1);
  TSLog("syncing rx1 done \n");

  //start children:
  TSLog("start children \n");
  for (i = 0; i < TC_NUM_CHILDREN; i++) {
    child_pids[i].pid = DoFork();
    if (child_pids[i].pid == 0) { 
      //child pid 
      args[i].master_pid = master_pid;
      args[i].dest       = &tc_rx_pids[0],
      args[i].num_dests  = TC_NUM_RX_PTHREADS;
      args[i].num_msgs   = TC_NUM_MSGS;
      args[i].im_a_child = true;
      (void)SendXMsgsThenExit((void *)(&args[i]));
    }
  }

  TSLog("kids created\n");

  //Wait for kids to start:
  for (i = 0; i < TC_NUM_CHILDREN; i++) {
    msg_t msg;
    msg.block = true;
    (void)RdOneMsg(&msg, MAILBOX_GOOD, "");
  }
  TSLog("kids started\n");

  //send msgs to kids to start them:
  for (i = 0; i < TC_NUM_CHILDREN; i++) {
    msg_t msg;
    TSLog("parent-start-msg to child #%d\n", child_pids[i].pid);
    snprintf(msg.buf, MAX_MSG_SIZE, "parent-start-msg to child #%d", 
		    child_pids[i].pid);
    (void)SendCharMsg(child_pids[i].pid, true, msg.buf, MAILBOX_GOOD);
  }

  //Wait for all rx threads to have started, and for this parent thread
  //to know this has occurred. 
  TSLog("syncing rx2\n");
  pthread_barrier_wait(&thread_barrier_2);
  TSLog("syncing rx2 done \n");

  //start tx threads:
  TSLog("starting send pthreads \n");
  for (i = 0; i < TC_NUM_TX_PTHREADS; i++) {
    j = (TC_NUM_CHILDREN + i);
    args[j].master_pid = master_pid;
    args[j].dest       = &tc_rx_pids[0];
    args[j].num_dests  = TC_NUM_RX_PTHREADS;
    args[j].num_msgs   = TC_NUM_MSGS;
    args[j].im_a_child = false;
    pthread_create(&tx_threads[i], NULL, SendXMsgsThenExit, 
		    (void *)(&args[j]));
  }

  TSLog("parent waiting for tx kids\n");
  //wait for kids and threads:
  WaitForChildrenToEnd(TC_NUM_CHILDREN, 0);
  TSLog("parent waiting for tx threads\n");
  WaitForThreadsToEnd(TC_NUM_TX_PTHREADS, tx_threads);

  //Once senders have stopped, we can stop the testing.
  TSLog("mailbox stopping\n");
  this_mbox_stopped = true;

  DoManage(true, &num_msgs);  
  TSLog("mailbox stopped\n");

  WaitForThreadsToEnd(TC_NUM_RX_PTHREADS, rx_threads);

  TSLog(TEST_PASSED);
}

//---------------------------------------------------------------------------
// InitExer
//---------------------------------------------------------------------------
void InitExer(tc_args_t *args){
unsigned int i;

  //called by top level only:
  args->max_num_msgs         = 100;
  args->max_num_kids         = 0;
  args->max_num_threads      = 10;
  args->num_levels           = 1;
  args->stop_exer            = false;
  args->num_threads_running  = 0;

  if ((args->num_levels      > MAX_NUM_LEVELS)  ||
      (args->max_num_kids    > MAX_NUM_KIDS)    ||
      (args->max_num_threads > MAX_NUM_THREADS) ||
      (args->max_num_msgs    > MAX_NUM_MSGS)) {
    Err("bad args for exer ");
  }
    

  //The threads do the msg handling, so we can't have zero:
  if (args->max_num_threads == 0)
    Err("max_num_threads != 0 ");

  args->level            = args->num_levels;
  args->num_dests        = 0;
  args->dest             = &exer_pid_list[0];
  args->parent_pid       = NULL_PID; //NULL_PID for top process
  for (i = 0; i < MAX_EPIDS; i++) {
	  args->dest[i].pid      = 0;
	  args->dest[i].msg_open = 0;
  }

}

//---------------------------------------------------------------------------
// Exer
// Start threads and children. 
// Send/rcv messages.
//
// This 'main' process waits for doneness.
// The threads do the msg passing. 
// The children spawn their own threads that do their msg passing.
// Wait for our children/threads to be done, then exit.
//
// Remember this is recursive: our children enter this also.
//---------------------------------------------------------------------------
void Exer(tc_args_t *args) {
pthread_t    my_threads[MAX_NUM_THREADS];
pid_t        my_kids[MAX_NUM_KIDS];
ex_args_t    ex_args[MAX_NUM_THREADS];
msg_t        msg;
unsigned int i;
unsigned int num_msgs;

  TSLog("EXER: level %d, pid %d \n", args->level, getpid());

  //We can enter this function as a child too, so we need to reinit the lock.
  pthread_mutex_init(&args->dest_lock, NULL);

  //set dynamic features:
  args->master_pid  = getpid();
  args->num_msgs    = (args->max_num_msgs == 0) ?
	                 0 : (rand() % args->max_num_msgs);
  args->num_threads = (rand() % (args->max_num_threads + 1));
  args->num_kids    = (args->max_num_kids == 0) ? 
	                 0 : (rand() % (args->max_num_kids) + 1);
  if ((args->level > 1) && (args->num_kids == 0)) args->num_kids = 1;
  else if (args->level == 1) args->num_kids = 0;
  if (args->num_msgs == 0) args->num_msgs = 1; //require one msg
  if (args->num_threads == 0) args->num_threads = 1; //require one thread

  TSLog("master_pid %d level %d kids %d threads %d msgs %d num_dests %d \n", 
	  args->master_pid, args->level, args->num_kids, args->num_threads,
	  args->num_msgs, args->num_dests);

  //Include us in these barriers too (the '+ 1'):
  if (pthread_barrier_init(&thread_barrier_1, NULL, (args->num_threads+1))) {
    Err("pthread_barrier_init failed %d \n", args->level);
  }
  if (pthread_barrier_init(&thread_barrier_2, NULL, (args->num_threads+1))) {
    Err("pthread_barrier_init failed %d \n", args->level);
  }


  TSLog("start pthreads level %d \n", args->level);
  //Start pthreads, before children, so we can easily pass the pthread ids 
  //to the children.
  for (i = 0; i < args->num_threads; i++) {
    ex_args[i].tc_args = args;
    ex_args[i].my_idx  = args->num_dests + i;
    pthread_create(&my_threads[i], NULL, DoExer, (void *)(&ex_args[i]));
  }
  //The args->dest list is longer because our threads get added into it.
  args->num_dests += args->num_threads;

  //Wait for all threads to have been created, and for this parent thread
  //to know this has occurred. This means that my_threads_pids[] is filled in
  //by my_threads and is good. 
  TSLog("syncing exer %d\n", args->level);
  pthread_barrier_wait(&thread_barrier_1);
  args->num_threads_running = args->num_threads;
  TSLog("syncing exer done %d, threads %d \n", 
	args->level, args->num_threads_running);


  if (args->num_kids != 0) {
    //start children (which in turn may start more threads/kids):
    TSLog("start children %d\n", args->level);
    for (i = 0; i < args->num_kids; i++) {
      my_kids[i] = DoFork();
      if (my_kids[i] == 0) { 
        //child pid 
	args->level--; 
        args->parent_pid = args->master_pid;
        args->im_a_child = true;
        (void)Exer((void *)(args));
      }
    }
    TSLog("kids created %d\n", args->level);


    //Wait for kids to start:
    for (i = 0; i < args->num_kids; i++) {
      msg_t msg;
      msg.block = true;
      (void)RdOneMsg(&msg, MAILBOX_GOOD, "");
    }
    TSLog("kids started %d\n", args->level);
  
  }

  if (args->parent_pid != NULL_PID) {
    //Tell parent that we've started:
    TSLog("level %d, child %d to parent %d exer start msg\n", 
	  args->level, getpid(), args->parent_pid);
    snprintf(msg.buf, MAX_MSG_SIZE, "exer start-msg to parent");
    (void)SendCharMsg(args->parent_pid, true, msg.buf, MAILBOX_GOOD);
  }

  //Tell threads to start:
  TSLog("syncing exer2 %d\n", args->level);
  pthread_barrier_wait(&thread_barrier_2);
  TSLog("syncing exer2 done %d\n", args->level);

  if (args->num_kids != 0) {
    //Wait for kids/grandkids to end first, because they are sending to our 
    //thread(s) also:
    TSLog("parent waiting for children %d\n", args->level);
    WaitForChildrenToEnd(args->num_kids, args->level);
  }

  //Tell our threads they can end (when their sending is done):
  args->stop_exer = true;
  TSLog("stopping threads %d \n", args->level);

  TSLog("parent waiting for threads %d\n", args->level);
  WaitForThreadsToEnd(args->num_threads, my_threads);

  //Once kids/threads have stopped, we can stop the testing.
  TSLog("mailbox stopping %d\n", args->level);
  DoManage(true, &num_msgs);  

  //Do a final cleanup of any pending msgs.
  RdUntilAck(getpid(), 0, args, true, false);

  if (args->level != args->num_levels) exit(EXIT_SUCCESS);
}

//---------------------------------------------------------------------------
// StartExer
// This is called by main.
//---------------------------------------------------------------------------
void StartExer() {
tc_args_t args;

  InitExer(&args);
  TSLog("master_pid %d, levels %d, max-kids %d, max-threads %d max-msgs %d \n", 
	  getpid(), args.num_levels, args.max_num_kids,
	  args.max_num_threads, args.max_num_msgs);
  Exer(&args);
  TSLog(TEST_PASSED);

}



//---------------------------------------------------------------------------
// main
//---------------------------------------------------------------------------
int main(int argc, char **argv) {
test_info_t  tinfo;
 
  printf("\n\n -- Susan Paradiso --\n\n");

  srand(time(NULL));

  this_mbox_stopped = false; 

  //Make the TSLog/print-lock mutex:
  if (pthread_mutex_init(&print_lock, NULL)) {
    printf("(main) pthread_mutex_init of print_lock failed\n");
    exit(EXIT_FAILURE);
  }

  HarvestArgs(argc, argv, &tinfo);

  switch (tinfo.test_num) {
	case 1:   TooLong(SLOW_MODE_ON);     break;
	case 2:   TooLong(SLOW_MODE_OFF);    break;
	case 3:   TooLong(TOO_LONG);         break;
	case 4:   VerifyAllMsgLens();        break;
	case 5:   TestGdMsgs();              break;
	case 6:   TestRcvBlkStoppedEmMbox(); break;
	case 7:   TestFullEmpty();           break;
	case 8:   TestFullWithChild();       break;
	case 9:   TestZeroLenMsg();          break;
	case 10:  TestBadSendArg();          break;
	case 11:  TestBadRcvArg();           break;
	case 12:  TestBadManageArg();        break;
	case 13:  TestSendPid1();            break;
	case 14:  TestThreadSharesMbox();    break;
	case 15:  TestDeletedMboxs(false);   break;
	case 16:  TestDeletedMboxs(true);    break;
	case 17:  TestThreadChildrenMsgs();  break;
	case 18:  TestMultipleStops();       break;
	case 19:  StartExer();               break;
	default:  Err("Bad arg, test_num %d does not exist", tinfo.test_num);
  }

  exit(EXIT_SUCCESS);
}


