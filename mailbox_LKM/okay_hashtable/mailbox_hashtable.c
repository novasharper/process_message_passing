/**
 * An OK hash table implementation (kernel space)
 * Uses slab allocator and stuff
 * @author Khazhismel Kumykov
 */

#include <linux/hash.h>

#include "mailbox_hashtable.h"

static struct hlist_head* mailbox_hash_table[MAILBOX_HASHTABLE_SIZE];

int init_mailbox_hashtable() {
    int i;
    for (i = 0; i < MAILBOX_HASHTABLE_SIZE; i++) {
        INIT_HLIST_HEAD(&mailbox_hash_table[i]);
    }
    return 0;
}

int cleanup_mailbox_hashtable() {
    return 0;
}

unsigned long hash_pid(pid_t pid) {
    return hash_64(pid, 64) % MAILBOX_HASHTABLE_SIZE;
}

struct hlist_node* hash_lookup(pid_t pid) {
    struct hlist_head * head;

    head = mailbox_hash_table[hash_pid(pid)];

    struct hlist_node current;
    Mailbox * m_current;

    hlist_for_each_entry(m_current, &current, head, list) {
        if (m_current->owner == pid) {
            return current;
        }
    }

    return NULL;
}

struct hlist_node* hash_put(pid_t pid_for, Mailbox* mailbox) {
    hlist_node* node = hash_lookup(pid_for);

    if (node) {
        // For now, don't support replacing stuff
        assert(false);
    } else {

    }
}