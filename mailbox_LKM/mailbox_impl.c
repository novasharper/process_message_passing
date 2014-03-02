/**
 * mailbox_impl.c
 * Mailboxes main implementation
 * @author Khazhismel Kumykov
 */

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>

#include "mailbox_impl.h"

struct kmem_cache* mailbox_cache;
struct kmem_cache* message_cache;

/**
 * Hash table stuff
 */

static struct hlist_head mailbox_hash_table[MAILBOX_HASHTABLE_SIZE];
DEFINE_RWLOCK(mailbox_hash_table_rwlock);
unsigned long flags;

static unsigned long mailbox_hash(pid_t owner) {
    // FIXME - this is a shitty hash function
    return owner % MAILBOX_HASHTABLE_SIZE;
}

/** Unsafe helper method, only use inside rwlock */
static Mailbox* __hashtable_get_unsafe(pid_t key) {
    struct hlist_node * current_node;
    Mailbox * m_current_node;

    hlist_for_each_entry(m_current_node, current_node, &mailbox_hash_table[mailbox_hash(key)], list) {
        if (m_current_node->owner == key) {
            return m_current_node;
        }
    }
    return NULL;
}

static Mailbox* hashtable_get(pid_t key) {
    Mailbox* search;
    read_lock_irqsave(&mailbox_hash_table_rwlock, flags);
    search = __hashtable_get_unsafe(key);
    read_unlock_irqrestore(&mailbox_hash_table_rwlock, flags);
    return search;
}

/** Unsafe helper method, only use inside rwlock */
static void __hashtable_remove_unsafe(pid_t key) {
    Mailbox* msg = __hashtable_get_unsafe(key);
    if (msg) {
        hlist_del(&msg->list);
    }
}

static void hashtable_remove(pid_t key) {
    write_lock_irqsave(&mailbox_hash_table_rwlock, flags);
    __hashtable_remove_unsafe(key);
    write_unlock_irqrestore(&mailbox_hash_table_rwlock, flags);
}

static void hashtable_put(Mailbox* mailbox, pid_t key) {
    write_lock_irqsave(&mailbox_hash_table_rwlock, flags);
    // Remove old entry if it exists
    __hashtable_remove_unsafe(key);

    // Add new entry
    hlist_add_head(&mailbox->list, &mailbox_hash_table[mailbox_hash(key)]);

    write_unlock_irqrestore(&mailbox_hash_table_rwlock, flags);
}

/**
 * Get the mailbox for a process, create it if it doesn't exist
 * @param  owner [description]
 * @return       [description]
 */
Mailbox * get_create_mailbox(pid_t owner) {
    Mailbox * mailbox = hashtable_get(owner);

    if (!mailbox) {
        mailbox = kmem_cache_alloc(mailbox_cache, 0);
        mailbox->owner = owner;
        mailbox->message_count = 0;
        mailbox->stopped = 0;
        INIT_LIST_HEAD(&mailbox->messages);
        INIT_HLIST_NODE(&mailbox->list);

        hashtable_put(mailbox, owner);
    }

    return mailbox;
}


/**
 * Destroys the mailbox and removes it from the hash table
 * @param owner [description]
 */
void destroy_mailbox(Mailbox* mailbox) {
    // Remove it from the hashtable
    hashtable_remove(mailbox->owner);

    // TODO - clean up messages

    // Free memory
    kmem_cache_free(mailbox_cache, mailbox);
}

void mailbox_add_message(Mailbox* mailbox, Message* message) {

}

void mailbox_remove_message(Mailbox* mailbox, Message* message) {

}

/**
 * Creates message and such
 *
 * @param  message the message
 * @param  sender who's sending
 * @param  len    Must be less than MAX_MSG_SIZE
 * @param  msg    Must be less than MAX_MSG_SIZE
 * @return        newly allocated Message
 */
long create_message(Message** message, pid_t sender, int len, void* msg) {
    if (len > MAX_MSG_SIZE) {
        return MSG_LENGTH_ERROR;
    }

    *message = kmem_cache_alloc(message_cache, 0);
    (*message)->sender = sender;
    (*message)->len = len;
    memcpy(&(*message)->msg, msg, len);
    INIT_LIST_HEAD(&(*message)->list);

    return 0;
}

long destroy_message(Message* message) {
    kmem_cache_free(message_cache, message);
    return 0;
}


long send_message_to_mailbox(pid_t reciever, pid_t sender, int len, void* msg) {
    Mailbox* mailbox = hashtable_get(reciever);

    // Mailbox not found, must be invalid
    if (mailbox == NULL) {
        return MAILBOX_INVALID;
    }

    return 0;
}

/**
 * Initalizes the mailbox implementation
 * Sets up the mem caches, and the hashmap, and the hashmaps and stuff
 */
void mailbox_impl_init() {
    int i;
    // Initialize our caches
    mailbox_cache = kmem_cache_create("mailbox", sizeof(Mailbox), 0, 0, NULL);
    message_cache = kmem_cache_create("message", sizeof(Message), 0, 0, NULL);

    // Initialize our hash table
    for (i = 0; i < MAILBOX_HASHTABLE_SIZE; i++) {
        INIT_HLIST_HEAD(&mailbox_hash_table[i]);
    }
}

/**
 * Cleans up
 */
void mailbox_impl_exit() {
    kmem_cache_destroy(mailbox_cache);
    kmem_cache_destroy(message_cache);
}
