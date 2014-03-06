/**
 * @file mailbox_manager.c
 *
 * @brief This file handles the mailbox table, requesting/destroying mailboxes for specific pids
 * validates pids, handles the hashtable, etc. 
 * Handles sync for hash table
 */

// We're in kernel space
#undef __KERNEL__
#undef MODULE

#define __KERNEL__ 
#define MODULE

#define MAILBOX_HASHTABLE_SIZE 4096

#include <linux/list.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/hash.h>

#include "mailbox_manager.h"
#include "mailbox.h"

// Initialize our hash table
static struct hlist_head mailbox_hash_table[MAILBOX_HASHTABLE_SIZE];
DEFINE_SPINLOCK(mailbox_hash_table_lock);
unsigned long flags;


void mailbox_manager_init() {
    int i;
    // Initialize our hash table
    for (i = 0; i < MAILBOX_HASHTABLE_SIZE; i++) {
        INIT_HLIST_HEAD(&mailbox_hash_table[i]);
    }

}

void mailbox_manager_exit() {
}

static unsigned long mailbox_hash(pid_t key) {
    return hash_32(key, 32) % MAILBOX_HASHTABLE_SIZE;
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


/**
 * A process is invalid if:
 * it's task_struct doesn't exist
 * it's cred uid is less than 1000 (kernel task)
 *  NOTE: task->mm == NULL is not a good test for kernel task, because a task can be interrupted while it's being created,
 *      and task->mm will be NULL, although it's a user task
 * it's flagged with exiting
 * it's dead
 *
 * @param  process [description]
 * @return         [description]
 */
static int is_process_valid(pid_t process) {
    struct task_struct* task = pid_task(find_get_pid(process), PIDTYPE_PID);

    if (task) {
        if (task->state == TASK_DEAD) {
            printk(KERN_INFO "Task %d is invalid because it's state is %ld dead", process, task->state);
            return 0;
        } else if(task->cred->uid < 1000) {
            printk(KERN_INFO "Task %d is invalid because it's a kernel task, uid %d", process, task->cred->uid);
            return 0;
        } else if(task->flags & PF_EXITING) {
            printk(KERN_INFO "Task %d is invalid because it's exiting", process);
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
    int error;
    unsigned long flags;
    if (is_process_valid(pid)) {
        spin_lock_irqsave(&mailbox_hash_table_lock, flags);
        *mailbox = __hashtable_get(pid);
        if (*mailbox == NULL) {
            error = mailbox_create(mailbox, pid);
            if (error) {
                spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
                return error;
            } else {
                __hashtable_put(pid, *mailbox);
                claim_mailbox(*mailbox);
                spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
                return 0;
            }
        } else {
            claim_mailbox(*mailbox);
            if ((*mailbox)->stopped & EXITING) { // Safe to read this because once exiting is set, stopped is never changed again
                unclaim_mailbox(*mailbox);
                *mailbox = NULL; // don't hand out the pointer
                spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
                return MAILBOX_INVALID;
            } else {
                spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
                return 0;
            }
        }
    } else {
        return MAILBOX_INVALID;
    }

}

/**
 * Runs under the principal that pids are not reused in short time
 * Waits until the task we're waiting on exits, then delete's it's mailbox
 * @param  arg [description]
 * @return     [description]
 */
int mailbox_deletion_thread(void* arg) {
    Mailbox* mailbox = (Mailbox*) arg;
    pid_t pid = mailbox->owner;
    unsigned long flags;

    while(is_process_valid(pid)) {
        printk(KERN_INFO "Task %d didn't exit yet, waiting", pid);
        msleep(50);
    }
    printk(KERN_INFO "Task %d exited, destroying mailbox", pid);

    spin_lock_irqsave(&mailbox_hash_table_lock, flags);
    __hashtable_remove(pid);
    spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
    mailbox_destroy(mailbox);

    printk(KERN_INFO "Mailbox for %d destroyed", pid);

    do_exit(0);
}

long remove_mailbox_for_pid(pid_t pid) {
    Mailbox* mailbox;
    unsigned long flags;

    spin_lock_irqsave(&mailbox_hash_table_lock, flags);
    mailbox = __hashtable_get(pid);

    if (mailbox) {
        printk(KERN_INFO "Stopping mailbox %d to prevent new messages, in preperation for destruction", pid);
        mailbox_exiting(mailbox);


        printk(KERN_INFO "Scheduling Mailbox %d for destruction", pid);
        

        spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
        kthread_run(&mailbox_deletion_thread, mailbox, "mailboxdestroy");
        return 0;
    } else {
        //printk(KERN_INFO "Tried to destroy non-existant mailbox %d", pid);
        spin_unlock_irqrestore(&mailbox_hash_table_lock, flags);
        return MAILBOX_INVALID;
    }
}

void claim_mailbox(Mailbox* m) {
    unsigned long flags;
    pid_t owner;
    int refs;
    spin_lock_irqsave(&m->dereference_queue.lock, flags);
    m->dereferences++;
    refs = m->dereferences;
    owner = m->owner;
    spin_unlock_irqrestore(&m->dereference_queue.lock, flags);
    printk(KERN_INFO "Mailbox for %d now has %d references (1 new claim)", owner, refs);
}

void unclaim_mailbox(Mailbox* m) {
    unsigned long flags;
    pid_t owner;
    int refs;
    spin_lock_irqsave(&m->dereference_queue.lock, flags);
    m->dereferences--;
    if (m->dereferences == 0) {
        wake_up_locked(&m->dereference_queue);
    }
    owner = m->owner;
    refs = m->dereferences;
    spin_unlock_irqrestore(&m->dereference_queue.lock, flags);
    printk(KERN_INFO "Mailbox for %d now has %d references (1 released claim)", owner, refs);
}

void wait_until_mailbox_unclaimed(Mailbox* m) {
    unsigned long flags;
    spin_lock_irqsave(&m->dereference_queue.lock, flags);
    wait_event_interruptible_exclusive_locked_irq(m->dereference_queue, 
                    m->dereferences == 0);
    spin_unlock_irqrestore(&m->dereference_queue.lock, flags);
}