#ifndef __MAILBOX_IMPL_P4__
#define __MAILBOX_IMPL_P4__


#include <linux/list.h>
#include <linux/wait.h>

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
    int message_max;            // Mailbox max size;
    int stopped;                // Is this mailbox stopped?
    struct list_head messages;  // Linked list of messages
    struct hlist_node list;     // I'm in a hash table
    spinlock_t lock;            // Modification/usage lock
    unsigned long lock_irqsave; // irqsave
    wait_queue_head_t send_recieve_message_queue; // Only send/recieve one message at a time, can't send and recieve.
} Mailbox;

void mailbox_impl_init(void);
void mailbox_impl_exit(void);

Mailbox * get_create_mailbox(pid_t owner);


void __init_message(Message** msg);
void __destroy_message(Message** msg);

// FIXME - all the _unsafe functions should be wrapped in thier own helpers, that way we don't have to handle sync code in the main mailbox.c
void __mailbox_add_message_unsafe(Mailbox* mailbox, Message* message);
void __mailbox_remove_message_unsafe(Mailbox* mailbox, Message* message);
void __mailbox_stop_unsafe(Mailbox* mailbox);

#endif