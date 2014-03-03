/**
 * @file mailbox_manager.c
 */

// We're in kernel space
#undef __KERNEL__
#undef MODULE

#define __KERNEL__ 
#define MODULE

#define MAILBOX_HASHTABLE_SIZE 4096

#include <linux/list.h>
#include <linux/sched.h>

#include "mailbox_manager.h"
#include "mailbox.h"

// Initialize our hash table
static struct hlist_head mailbox_hash_table[MAILBOX_HASHTABLE_SIZE];
DEFINE_RWLOCK(mailbox_hash_table_rwlock);
unsigned long flags;

void mailbox_manager_init() {
    int i;
    // Initialize our hash table
    for (i = 0; i < MAILBOX_HASHTABLE_SIZE; i++) {
        INIT_HLIST_HEAD(&mailbox_hash_table[i]);
    }
}

static unsigned long mailbox_hash(pid_t key) {
    // FIXME - this isn't a very good hash function
    return key % MAILBOX_HASHTABLE_SIZE;
}

static Mailbox* __hashtable_get(pid_t key) {
    struct hlist_node* node;
    Mailbox* mailbox;

    hlist_for_each_entry(mailbox, node, &mailbox_hash_table[mailbox_hash(key)], hash_table_entry) {
        if (mailbox->owner == key) {
            return mailbox;
        }
    }

    return NULL;
}

static void __hashtable_remove(pid_t key) {
    Mailbox* mailbox = __hashtable_get(key);
    if (mailbox) {
        hlist_del(&mailbox->hash_table_entry);
    }
}

static void __hashtable_put(pid_t key, Mailbox* mailbox) {
    __hashtable_remove(key);
    hlist_add_head(&mailbox->hash_table_entry, &mailbox_hash_table[mailbox_hash(key)]);
}


Mailbox* hashtable_get(pid_t key) {
    Mailbox* search;
    unsigned long flags;
    read_lock_irqsave(&mailbox_hash_table_rwlock, flags);
    search = __hashtable_get(key);
    read_unlock_irqrestore(&mailbox_hash_table_rwlock, flags);
    return search;
}

void hashtable_put(pid_t key, Mailbox* mailbox) {
    unsigned long flags;
    write_lock_irqsave(&mailbox_hash_table_rwlock, flags);
    __hashtable_put(key, mailbox);
    write_unlock_irqrestore(&mailbox_hash_table_rwlock, flags);
}

void hashtable_remove(pid_t key) {
    write_lock_irqsave(&mailbox_hash_table_rwlock, flags);
    __hashtable_remove(key);
    write_unlock_irqrestore(&mailbox_hash_table_rwlock, flags);
}

int is_process_valid(pid_t process) {
    struct task_struct* task = pid_task(find_get_pid(process), PIDTYPE_PID);

    if (task) {
        if (task->state == TASK_STOPPED) {
            printk(KERN_INFO "Task %d is invalid because it's state is %ld", process, task->state);
            return 0;
        } else if(task->mm) {
            printk(KERN_INFO "Task %d is invalid because it's a kernel task", process);
            return 0;
        } else {
            printk(KERN_INFO "Task %d is valid because it's state is %ld", process, task->state);
            return 1;
        }
    } else {
        printk(KERN_INFO "Task %d is invalid because it doesn't exist", process);
        return 0;
    }
}

long get_mailbox_for_pid(Mailbox** mailbox, pid_t pid) {
    if (is_process_valid(pid)) {
        *mailbox = hashtable_get(pid);
        if (*mailbox == NULL) {
            *mailbox = mailbox_create(pid);
            hashtable_put(pid, *mailbox);
        }
        return 0;
    } else {
        return MAILBOX_INVALID;
    }
}

long remove_mailbox_for_pid(pid_t pid) {
    Mailbox* mailbox;

    mailbox = hashtable_get(pid);

    if (mailbox) {
        hashtable_remove(pid);
        mailbox_destroy(mailbox);
        return 0;
    } else {
        return MAILBOX_INVALID;
    }
}