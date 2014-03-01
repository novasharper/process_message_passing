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
	char msg[MAX_MESSAGE_SIZE]; // Message data
	struct list_head list;
} Message;

typedef struct Mailbox {
	pid_t id;
	bool stopped;
	kmem_cache_t *mem_cache;
	struct Message message_list;
	int message_count;
	wait_queue_head_t *read_queue;
	wait_queue_head_t *write_queue;
	// No messages wait event ...
} Mailbox;

wait_queue_head_t *mailboxes_ht_wq;
hashtab_t *mailboxes;

int TRIGGER = 0;

void init_mailboxes(void);
Mailbox *create_mailbox(pid_t proc_id);
long create_message(pid_t proc_id, Message *new_message);
long append_message(pid_t proc_id, Message *message);
long get_message(pid_t proc_id, Message *message);
void destroy_message(pid_t proc_id, Message *to_delete);
void destroy_mailbox(pid_t proc_id);
// Get mailbox by process ID
long get_mailbox(pid_t proc_id, Mailbox *mailbox);

#endif