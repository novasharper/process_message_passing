#ifndef __MAILBOX_MANAGER__
#define __MAILBOX_MANAGER__

#define NO_BLOCK 0
#define BLOCK 1

#include "mailbox.h"

void mailbox_manager_init(void);

long get_mailbox_for_pid(Mailbox** mailbox, pid_t pid);
long mailbox_stop_for_pid(pid_t pid);
long remove_mailbox_for_pid(pid_t pid);

#endif