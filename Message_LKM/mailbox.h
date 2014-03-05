#ifndef __MAILBOX__
#define __MAILBOX__


/**
 * error codes pertaining to mailboxes
 * 
 * */
#define MAILBOX_FULL        1001
#define MAILBOX_EMPTY       1002
#define MAILBOX_STOPPED     1003
#define MAILBOX_INVALID     1004
#define MAILBOX_ERROR       1007

#include <linux/wait.h>

#include "message.h"

#define STOPPED 0x01
#define EXITING 0x02    // exiting and stopped

typedef struct __mailbox {
    pid_t owner;                                    // Owner of this mailbox
    int message_count;                              // Number of messages in this mailbox
    int message_max;                                // Mailbox max size;
    int stopped;                                    // Is this mailbox stopped? Also used for exiting
    struct list_head messages;                      // Linked list of messages (We are not in this list)
    struct hlist_node hash_table_entry;             // Hash Table Entry (for mailbox_manager)
    wait_queue_head_t modify_queue;                 // Lock for this mailbox, and a queue
    atomic_t waiting;                               // How many processes waiting to use this mailbox
    wait_queue_head_t dereference_queue;            // Dereferences, handled by mm
    int dereferences;                               // uses dereference_queue lock
} Mailbox;

void mailbox_init(void);
void mailbox_exit(void);

long mailbox_create(Mailbox** mailbox, pid_t owner);

long mailbox_add_message(Mailbox* mailbox, Message* message, int block, int head);
long mailbox_remove_message(Mailbox* mailbox, Message** message, int block);
long mailbox_stop(Mailbox* mailbox);
long mailbox_exiting(Mailbox* mailbox);
long mailbox_destroy(Mailbox* mailbox);

#endif