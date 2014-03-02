/**
 * mailbox_impl.c
 * Mailboxes main implementation
 * @author Khazhismel Kumykov
 * @author Pat Long
 */

#include <linux/slab.h>
#include <linux/list.h>

#include "mailbox_impl.h"

struct kmem_cache* mailbox_cache;
struct kmem_cache* message_cache;

static struct hlist_head mailbox_hash_table[MAILBOX_HASHTABLE_SIZE];

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

unsigned long mailbox_hash(pid_t owner) {
    // FIXME - this is a shitty hash function
    return owner % MAILBOX_HASHTABLE_SIZE;
}

Mailbox* get_mailbox(pid_t owner) {
    struct hlist_head head = mailbox_hash_table[mailbox_hash(owner)];

    struct hlist_node * current_node;
    Mailbox * m_current;

    hlist_for_each_entry(m_current, current_node, &head, list) {
        if (m_current->owner == owner) {
            return m_current;
        }
    }

    return NULL;
}

/**
 * Creates a mailbox for the given owner
 * @param  mailbox Pointer to Mailbox that we'll overwrite for you
 * @param  owner   pid of owner
 * @return         0
 */
Mailbox * create_mailbox(pid_t owner) {
    Mailbox * mailbox;

    if(get_mailbox(owner) != NULL) {
        return NULL;
    }

    mailbox = kmem_cache_alloc(mailbox_cache, 0);
    mailbox->owner = owner;
    mailbox->message_count = 0;
    mailbox->stopped = 0;
    INIT_LIST_HEAD(&mailbox->messages);
    INIT_HLIST_NODE(&mailbox->list);

    // Assume it's not already created
    hlist_add_head(&mailbox->list, &mailbox_hash_table[mailbox_hash(owner)]);

    return mailbox;
}

void destroy_mailbox(pid_t owner) {
    Mailbox * mailbox;

    mailbox = get_mailbox(owner);

    if (!mailbox) {
        return;
    }

    // Remove it from the hashtable
    hlist_del(&mailbox->list);

    // Free memory
    kmem_cache_free(mailbox_cache, mailbox);


}