#ifndef __MAILBOX_MANAGER__
#define __MAILBOX_MANAGER__

#define NO_BLOCK 0
#define BLOCK 1

#include <linux/wait.h>
#include "mailbox.h"

void mailbox_manager_init(void);
void mailbox_manager_exit(void);

long get_mailbox_for_pid(Mailbox** mailbox, pid_t pid);
long remove_mailbox_for_pid(pid_t pid);

void claim_mailbox(Mailbox* m);
void unclaim_mailbox(Mailbox* m);
void wait_until_mailbox_unclaimed(Mailbox* m);

#endif