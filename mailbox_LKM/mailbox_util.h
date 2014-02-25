#ifndef _MAILBOX_UTIL__H
#define _MAILBOX_UTIL__H

#include "hashtab.h"
#include "mailbox.h"

pthread_rwlock_t lock;

typedef struct Message {
	int sender;
	int len;
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
} Mailbox;

hashtab_t *mailboxes;

void init_mailboxes(void);
void create_mailbox(pid_t proc_id);
Message *create_message(pid_t proc_id);
void destoroy_message(pid_t proc_id, Message *to_delete);
void destroy_mailbox(pid_t proc_id);
// Get mailbox by process ID
Mailbox *get_mailbox(pid_t proc_id);

#endif