#ifndef _MAILBOX_UTIL__H
#define _MAILBOX_UTIL__H

#include <linux/wait.h>
#include <linux/sched.h>
#include "hashtab.h"
#include "mailbox.h"

typedef struct Message {
	int sender;
	int len;
	bool dirty;
	struct Message *next;
	void *msg;
} Message;

typedef struct Mailbox {
	pid_t id;
	bool stopped;
	kmem_cache_t *mem_cache;
	struct Message *start;
	struct Message *tail;
	int message_count;
	wait_queue_head_t *read_queue;
	wait_queue_head_t *write_queue;
} Mailbox;

hashtab_t *mailboxes;

void init_mailboxes(void);
void create_mailbox(pid_t proc_id);
Message *create_message(pid_t proc_id);
void append_message(pid_t proc_id, Message *message);
Message *get_message(pid_t proc_id);
void destroy_message(pid_t proc_id, Message *to_delete);
void destroy_mailbox(pid_t proc_id);
// Get mailbox by process ID
Mailbox *get_mailbox(pid_t proc_id);

#endif