#ifndef __MAILBOX_MANAGER__
#define __MAILBOX_MANAGER__

#define NO_BLOCK 0
#define BLOCK 1

#include "mailbox.h"

int is_process_valid(pid_t process);

void mailbox_manager_init(void);

long get_mailbox_for_pid(Mailbox** mailbox, pid_t pid);
long remove_mailbox_for_pid(pid_t pid);

#endif