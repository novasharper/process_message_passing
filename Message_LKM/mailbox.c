/**
 * @file mailbox.c
 *
 * @brief this file handles syncronization for individual mailboxes, creation and destruction
 */

// We're in kernel space
#undef __KERNEL__
#undef MODULE

#define __KERNEL__ 
#define MODULE

#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/list.h>

#include "mailbox.h"
#include "message.h"
#include "mailbox_manager.h"

static struct kmem_cache* mailbox_cache;

/**
 * Initializes our kmem cache
 */
void mailbox_init() {
    mailbox_cache = kmem_cache_create("mailbox", sizeof(Mailbox), 0, 0, NULL);
}

/**
 * Destroys our kmem cache
 */
void mailbox_exit() {
    kmem_cache_destroy(mailbox_cache);
}

/**
 * Creates a mailbox for given pid. (does not put it anywhere)
 * @param  mailbox mailbox
 * @param  owner given pid
 * @return       error code
 */
long mailbox_create(Mailbox** mailbox, pid_t owner) {
    printk(KERN_INFO "Creating new mailbox for %d", owner);

    // Allocate it
    *mailbox = kmem_cache_alloc(mailbox_cache, 0);

    if (!(*mailbox)) {
        printk(KERN_ALERT "Unable to allocate memory for mailbox!");
        return MAILBOX_ERROR;
    }

    // Initialize info
    (*mailbox)->owner = owner;
    (*mailbox)->message_count = 0;
    (*mailbox)->message_max = 32; // Magic constant
    (*mailbox)->stopped = 0;
    atomic_set(&(*mailbox)->waiting, 0);

    // Initialize lists
    INIT_LIST_HEAD(&(*mailbox)->messages);
    INIT_HLIST_NODE(&(*mailbox)->hash_table_entry);

    // Initialize the wait queue
    init_waitqueue_head(&(*mailbox)->modify_queue);

    init_waitqueue_head(&(*mailbox)->dereference_queue);
    (*mailbox)->dereferences = 0;

    return 0;
}

/**
 * Util function if mailbox is full
 * @param  mailbox [description]
 * @return         [description]
 */
static int mailbox_full(Mailbox* mailbox) {
    return (mailbox->message_count >= mailbox->message_max);
}

/**
 * Util function, lock mailbox, print message
 * @param mailbox [description]
 * @param flags   [description]
 */
#define mailbox_lock(mailbox, flags) { \
    spin_lock_irqsave(&mailbox->modify_queue.lock, flags); \
    printk(KERN_INFO  "Mailbox %d: Spin locked by %d, Flags: %lu", mailbox->owner, current->tgid, flags); \
}

/**
 * Util function, unlock mailbox, print message
 * @param mailbox [description]
 * @param flags   [description]
 */
#define mailbox_unlock(mailbox, flags) { \
    spin_unlock_irqrestore(&mailbox->modify_queue.lock, flags); \
    printk(KERN_INFO  "Mailbox %d: Spin released by %d, Flags: %lu", mailbox->owner, current->tgid, flags); \
}

/**
 * Adds a message to this mailbox safely
 * @param  mailbox [description]
 * @param  message message to add
 * @param  block   [description]
 * @param  head    boolean - true add to head of list, false add to end of list (default end, the head is used to re-add a message that was unsuccesfully copied to preserve FIFO)
 * @return         error
 */
long mailbox_add_message(Mailbox* mailbox, Message* message, int block, int head) {
    unsigned long flags;

    mailbox_lock(mailbox, flags);

    //printk(KERN_INFO  "Mailbox %d: Adding message from %d", mailbox->owner, message->sender);

    if (mailbox->stopped) {
        printk(KERN_INFO "Mailbox %d: Mailbox is stopped, cannot send messages", mailbox->owner);
        mailbox_unlock(mailbox, flags);
        return MAILBOX_STOPPED;
    } else {
        if (mailbox_full(mailbox)) {
            if (block) {
                printk(KERN_INFO "Mailbox %d: Mailbox full, waiting until open space", mailbox->owner);
                atomic_inc(&mailbox->waiting);
                if (wait_event_interruptible_exclusive_locked_irq(mailbox->modify_queue, 
                    (mailbox->stopped || !mailbox_full(mailbox)))) {
                    printk(KERN_INFO "Recieved signal, exiting...");
                    atomic_dec(&mailbox->waiting);
                    wake_up_locked(&mailbox->modify_queue);
                    mailbox_unlock(mailbox, flags);
                    return MAILBOX_ERROR;
                }
                atomic_dec(&mailbox->waiting);
                wake_up_locked(&mailbox->modify_queue);
                if (mailbox->stopped) {
                    printk(KERN_INFO "Mailbox %d: Mailbox became stopped while waiting to add, cannot add", mailbox->owner);

                    mailbox_unlock(mailbox, flags);
                    return MAILBOX_STOPPED;
                }
            } else {
                printk(KERN_INFO "Mailbox %d: Mailbox full, not adding message", mailbox->owner);

                mailbox_unlock(mailbox, flags);
                return MAILBOX_FULL;
            }
        }

        // Add to the end of the list
        if (head) {
            list_add(&message->list, &mailbox->messages);
        } else {
            list_add_tail(&message->list, &mailbox->messages);
        }
        mailbox->message_count++;
        wake_up_locked(&mailbox->modify_queue);

        printk(KERN_INFO  "Mailbox %d: Message from %d added successfully", mailbox->owner, message->sender);

        mailbox_unlock(mailbox, flags);
        return 0;
    }
}

/**
 * Removes a message from this mailbox safely
 * @param  mailbox [description]
 * @param  message pointer for us to set
 * @param  block   [description]
 * @return         error
 */
long mailbox_remove_message(Mailbox* mailbox, Message** message, int block) {
    unsigned long flags;

    mailbox_lock(mailbox, flags);

    printk(KERN_INFO "Mailbox %d: Removing first message from mailbox", mailbox->owner);

    if (mailbox->stopped && mailbox->message_count == 0) {
        printk(KERN_INFO "Mailbox %d: Mailbox is stopped, and there are no more messages to recieve", mailbox->owner);

        mailbox_unlock(mailbox, flags);
        return MAILBOX_STOPPED;
    } else {
        if (mailbox->message_count == 0) {
            // Wait if we want to, otherwise don't
            if (block) {
                printk(KERN_INFO "Mailbox %d: Mailbox empty, waiting until message arrives", mailbox->owner);
                // Wait until there's a message we want to see, or the mailbox is stopped
                atomic_inc(&mailbox->waiting);
                if (wait_event_interruptible_exclusive_locked_irq(mailbox->modify_queue,
                    (mailbox->stopped || mailbox->message_count != 0))) {
                    printk(KERN_INFO "Recieved signal, exiting...");
                    atomic_dec(&mailbox->waiting);
                    wake_up_locked(&mailbox->modify_queue);
                    mailbox_unlock(mailbox, flags);
                    return MAILBOX_ERROR;
                }
                atomic_dec(&mailbox->waiting);
                wake_up_locked(&mailbox->modify_queue);
                if (mailbox->stopped && mailbox->message_count == 0) {
                    printk(KERN_INFO "Mailbox %d: Mailbox became stopped and empty while we were waiting", mailbox->owner);

                    mailbox_unlock(mailbox, flags);
                    return MAILBOX_STOPPED;
                }
            } else {
                printk(KERN_INFO "Mailbox %d: Mailbox empty, no message recieved", mailbox->owner);

                mailbox_unlock(mailbox, flags);
                return MAILBOX_EMPTY;
            }
        }

        // Get the message
        *message = list_entry(mailbox->messages.next, Message, list);
        // Remove it from the list
        list_del_init(&(*message)->list);
        mailbox->message_count--;
        wake_up_locked(&mailbox->modify_queue);

        printk(KERN_INFO  "Mailbox %d: Successfully got message from %d", mailbox->owner, (*message)->sender);

        mailbox_unlock(mailbox, flags);
        return 0;
    }
}

/**
 * Util method to stop mailbox safely, sets flags
 * @param  mailbox [description]
 * @param  status  [description]
 * @return         [description]
 */
static long __mailbox_stop(Mailbox* mailbox, int status) {
    unsigned long flags;

    mailbox_lock(mailbox, flags);

    mailbox->stopped = status;
    printk(KERN_INFO "Mailbox %d: Waking up everything", mailbox->owner);
    wake_up_locked(&mailbox->modify_queue);

    mailbox_unlock(mailbox, flags);

    printk(KERN_INFO "Mailbox %d: Mailbox Stopped", mailbox->owner);
    return 0;
}

/**
 * Stop the mailbox normally
 * @param  mailbox [description]
 * @return         [description]
 */
long mailbox_stop(Mailbox* mailbox) {
    return __mailbox_stop(mailbox, STOPPED);
}

/**
 * Stop the mailbox and mark the mailbox as about to be deleted
 * @param  mailbox [description]
 * @return         [description]
 */
long mailbox_exiting(Mailbox* mailbox) {
    return __mailbox_stop(mailbox, EXITING | STOPPED);
}

/**
 * Destroy the mailbox. This stops, waits, flushes, waits for pointers to be unclaimed, then frees the mailbox
 * @param  mailbox [description]
 * @return         [description]
 */
long mailbox_destroy(Mailbox* mailbox) {
    Message *msg, *next_msg;
    unsigned long flags;

    printk(KERN_INFO "Mailbox %d: Destroying, stopping", mailbox->owner);
    printk(KERN_INFO "Mailbox %d: Stopping mailbox with %d left waiting", mailbox->owner, atomic_read(&mailbox->waiting));
    mailbox_stop(mailbox);

    printk(KERN_INFO "Mailbox %d: Trying to get mailbox lock", mailbox->owner);
    
    
    mailbox_lock(mailbox, flags);


    printk(KERN_INFO "Mailbox %d: Waiting until other processes finish with this mailbox", mailbox->owner);
    wait_event_interruptible_locked_irq(mailbox->modify_queue, /* printk(KERN_INFO  "Mailbox %d is Still waiting, %d left", mailbox->owner, atomic_read(&mailbox->waiting)) && */(atomic_read(&mailbox->waiting) == 0));

    printk(KERN_INFO "Mailbox %d: Flushing messages", mailbox->owner);

    list_for_each_entry_safe(msg, next_msg, &mailbox->messages, list) {
        list_del(&msg->list);
        message_destroy(&msg);
    }

    printk(KERN_INFO "Mailbox %d: No more messages, destroying mailbox", mailbox->owner);

    mailbox_unlock(mailbox, flags);
    wait_until_mailbox_unclaimed(mailbox);
    printk(KERN_INFO "Mailbox %d is empty on delete: %d", mailbox->owner, list_empty(&mailbox->messages));
    kmem_cache_free(mailbox_cache, mailbox);
    
    return 0;
}
