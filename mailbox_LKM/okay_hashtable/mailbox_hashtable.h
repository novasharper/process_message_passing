#ifndef __MAILBOX_HASHTABLE_PR4__
#define __MAILBOX_HASHTABLE_PR4__

#include <linux/list.h>

#define MAILBOX_HASHTABLE_SIZE 4096

int init_mailbox_hashtable(void);
int cleanup_mailbox_hashtable(void);

#endif