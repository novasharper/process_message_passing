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
    // need to add syncronization here incase two processes try to get/create at the same time
    Mailbox * mailbox = hashtable_get(owner);

    if (!mailbox) {
        mailbox = kmem_cache_alloc(mailbox_cache, 0);
        mailbox->owner = owner;
        mailbox->message_count = 0;
        mailbox->stopped = 0;
        spin_lock_init(&mailbox->lock);
        init_waitqueue_head(&mailbox->send_recieve_message_queue);
        INIT_LIST_HEAD(&mailbox->messages);
        INIT_HLIST_NODE(&mailbox->list);

        // Put it in the table
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

    // Free memory
    kmem_cache_free(mailbox_cache, mailbox);

    // Free all messages
    // ...
}


/** Mailbox modification functions, adding and removing messages, setting to stopped */

void __mailbox_add_message_unsafe(Mailbox* mailbox, Message* message) {
    spin_lock_irqsave(&mailbox->lock, mailbox->lock_irqsave);
    // Add the message to the end of the list
    list_add_tail(&message->list, &mailbox->messages);
    mailbox->message_count++;
    spin_unlock_irqrestore(&mailbox->lock, mailbox->lock_irqsave);
}

void __mailbox_remove_message_unsafe(Mailbox* mailbox, Message* message) {
    spin_lock_irqsave(&mailbox->lock, mailbox->lock_irqsave);
    if(&message->list == &mailbox->messages) mailbox->messages = *mailbox->messages.next;
    list_del(&message->list);
    mailbox->message_count--;
    spin_unlock_irqrestore(&mailbox->lock, mailbox->lock_irqsave);
}

void __mailbox_stop_unsafe(Mailbox* mailbox) {
    spin_lock_irqsave(&mailbox->lock, mailbox->lock_irqsave);
    mailbox->stopped = 1;
    spin_unlock_irqrestore(&mailbox->lock, mailbox->lock_irqsave);
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
void __init_message(Message** message) {
    *message = kmem_cache_alloc(message_cache, 0);
    INIT_LIST_HEAD(&(*message)->list);
}

void __destroy_message(Message** message) {
    kmem_cache_free(message_cache, message);
    *message = NULL;
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
