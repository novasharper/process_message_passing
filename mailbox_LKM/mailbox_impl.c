/**
 * mailbox_impl.c
 * Mailboxes main implementation
 * @author Khazhismel Kumykov
 * @author Pat Long
 */

/* To go in the header */

#define MAILBOX_HASHTABLE_SIZE 4096

typedef struct Message {
    pid_t sender;               // Record the sender in the message
    int len;                    // The length of the message, cannot exceed MAX_MESSAGE_SIZE
    char msg[MAX_MESSAGE_SIZE]; // The message itself
    struct list_head list;      // Linked list
} Message;

typedef struct Mailbox {
    pid_t owner;                // Owner of this mailbox
    struct list_head messages;  // Linked list of messages
    int message_count;          // Not greater than MAILBOX_SIZE
    int stopped;               // Is this mailbox stopped?
    struct hlist_node list;
} Mailbox;

/* End Header shiz */

#include <linux/list.h>
#include "mailbox_impl.h"

kmem_cache_t* mailbox_cache;
kmem_cache_t* message_cache;

/**
 * Initalizes the mailbox implementation
 * Sets up the mem caches, and the hashmap, and the hashmaps and stuff
 */
void mailbox_impl_init() {
    // Initialize our caches
    mailbox_cache = kmem_cache_create("mailbox", sizeof(Mailbox), 0, 0, NULL);
    message_cache = kmem_cache_create("message", sizeof(Message), 0, 0, NULL);

    // Initialize our hashmaps
    assert(false); // Implement me!
}

/**
 * Creates a mailbox for the given owner
 * @param  mailbox Pointer to Mailbox that we'll overwrite for you
 * @param  owner   pid of owner
 * @return         0
 */
int create_mailbox(Mailbox** mailbox, pid_t owner) {
    mailbox* = kmem_cache_alloc(&mailbox_cache, 0);
    mailbox*->owner = owner;
    mailbox*->message_count = 0;
    mailbox*->stopped = 0;
}

int create_message(Message** message, pid_t sender, void)