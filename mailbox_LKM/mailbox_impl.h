#ifndef __MAILBOX_IMPL_P4__
#define __MAILBOX_IMPL_P4__


#include <linux/list.h>

/** Error codes that must match userspace mailbox.h */
#define NO_BLOCK 0
#define BLOCK   1
#define MAX_MSG_SIZE 128

/**
 * error codes pertaining to mailboxes
 * 
 * */
#define MAILBOX_FULL        1001
#define MAILBOX_EMPTY       1002
#define MAILBOX_STOPPED     1003
#define MAILBOX_INVALID     1004
#define MSG_LENGTH_ERROR    1005
#define MSG_ARG_ERROR       1006
#define MAILBOX_ERROR       1007



#define MAILBOX_HASHTABLE_SIZE 4096

typedef struct Message {
    pid_t sender;               // Record the sender in the message
    int len;                    // The length of the message, cannot exceed MAX_MESSAGE_SIZE
    char msg[MAX_MSG_SIZE]; // The message itself
    struct list_head list;      // Linked list
} Message;

typedef struct Mailbox {
    pid_t owner;                // Owner of this mailbox
    int message_count;          // Not greater than MAILBOX_SIZE
    int stopped;                // Is this mailbox stopped?
    struct list_head messages;  // Linked list of messages
    struct hlist_node list;     // I'm in a hash table
    spinlock_t lock;            // Modification/usage lock
    unsigned long lock_irqsave; // irqsave
    // TODO  - locks per mailbox
} Mailbox;

void mailbox_impl_init(void);
void mailbox_impl_exit(void);

Mailbox * get_create_mailbox(pid_t owner);

#endif