/**
 * @file mailbox.c
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

struct kmem_cache* mailbox_cache;

void mailbox_init() {
    mailbox_cache = kmem_cache_create("mailbox", sizeof(Mailbox), 0, 0, NULL);
}

void mailbox_exit() {
    kmem_cache_destroy(mailbox_cache);
}

Mailbox* mailbox_create(pid_t owner) {
    Mailbox* mailbox;

    printk(KERN_INFO "Creating new mailbox for %d", owner);

    // Allocate it
    mailbox = kmem_cache_alloc(mailbox_cache, 0);

    // Initialize info
    mailbox->owner = owner;
    mailbox->message_count = 0;
    mailbox->message_max = 32; // Magic constant
    mailbox->stopped = 0;
    atomic_set(&mailbox->waiting, 0);

    // Initialize lists
    INIT_LIST_HEAD(&mailbox->messages);
    INIT_HLIST_NODE(&mailbox->hash_table_entry);

    // Initialize the wait queue
    init_waitqueue_head(&mailbox->modify_queue);

    return mailbox;
}

int mailbox_full(Mailbox* mailbox) {
    return (mailbox->message_count >= mailbox->message_max);
}

void mailbox_lock(Mailbox* mailbox, unsigned long* flags) {
    spin_lock_irqsave(&mailbox->modify_queue.lock, *flags);
    printk(KERN_INFO "Mailbox %d: Spin locked", mailbox->owner);
}

void mailbox_unlock(Mailbox* mailbox, unsigned long* flags) {
    spin_unlock_irqrestore(&mailbox->modify_queue.lock, *flags);
    printk(KERN_INFO "Mailbox %d: Spin released", mailbox->owner);
}

long mailbox_add_message(Mailbox* mailbox, Message* message, int block) {
    unsigned long flags;

    mailbox_lock(mailbox, &flags);

    printk(KERN_INFO "Mailbox %d: Adding message from %d", mailbox->owner, message->sender);

    if (mailbox->stopped) {
        printk(KERN_INFO "Mailbox %d: Mailbox is stopped, cannot send messages", mailbox->owner);
        mailbox_unlock(mailbox, &flags);
        return MAILBOX_STOPPED;
    } else {
        if (mailbox_full(mailbox)) {
            if (block) {
                printk(KERN_INFO "Mailbox %d: Mailbox full, waiting until open space", mailbox->owner);
                atomic_inc(&mailbox->waiting);
                wait_event_interruptible_exclusive_locked_irq(mailbox->modify_queue, 
                    (mailbox->stopped || !mailbox_full(mailbox)));
                atomic_dec(&mailbox->waiting);
                if (mailbox->stopped) {
                    printk(KERN_INFO "Mailbox %d: Mailbox became stopped while waiting to add, cannot add", mailbox->owner);

                    mailbox_unlock(mailbox, &flags);
                    return MAILBOX_STOPPED;
                }
            } else {
                printk(KERN_INFO "Mailbox %d: Mailbox full, not adding message", mailbox->owner);

                mailbox_unlock(mailbox, &flags);
                return MAILBOX_FULL;
            }
        }

        // Add to the end of the list
        list_add_tail(&message->list, &mailbox->messages);
        mailbox->message_count++;
        wake_up_locked(&mailbox->modify_queue);

        printk(KERN_INFO "Mailbox %d: Message from %d added successfully", mailbox->owner, message->sender);

        mailbox_unlock(mailbox, &flags);
        return 0;
    }
}

long mailbox_remove_message(Mailbox* mailbox, Message** message, int block) {
    unsigned long flags;

    mailbox_lock(mailbox, &flags);

    printk(KERN_INFO "Mailbox %d: Removing first message from mailbox", mailbox->owner);

    if (mailbox->stopped && mailbox->message_count == 0) {
        printk(KERN_INFO "Mailbox %d: Mailbox is stopped, and there are no more messages to recieve", mailbox->owner);

        mailbox_unlock(mailbox, &flags);
        return MAILBOX_STOPPED;
    } else {
        if (mailbox->message_count == 0) {
            // Wait if we want to, otherwise don't
            if (block) {
                printk(KERN_INFO "Mailbox %d: Mailbox empty, waiting until message arrives", mailbox->owner);
                // Wait until there's a message we want to see, or the mailbox is stopped
                atomic_inc(&mailbox->waiting);
                wait_event_interruptible_exclusive_locked_irq(mailbox->modify_queue,
                    (mailbox->stopped || mailbox->message_count != 0));
                atomic_dec(&mailbox->waiting);
                if (mailbox->stopped && mailbox->message_count == 0) {
                    printk(KERN_INFO "Mailbox %d: Mailbox became stopped and empty while we were waiting", mailbox->owner);

                    mailbox_unlock(mailbox, &flags);
                    return MAILBOX_STOPPED;
                }
            } else {
                printk(KERN_INFO "Mailbox %d: Mailbox empty, no message recieved", mailbox->owner);

                mailbox_unlock(mailbox, &flags);
                return MAILBOX_EMPTY;
            }
        }

        // Get the message
        *message = list_entry(mailbox->messages.next, Message, list);
        // Remove it from the list
        list_del(&(*message)->list);
        mailbox->message_count--;
        wake_up_locked(&mailbox->modify_queue);

        printk(KERN_INFO "Mailbox %d: Successfully got message from %d", mailbox->owner, (*message)->sender);

        mailbox_unlock(mailbox, &flags);
        return 0;
    }
}

long mailbox_stop(Mailbox* mailbox) {
    unsigned long flags;

    mailbox_lock(mailbox, &flags);

    mailbox->stopped = 1;

    printk(KERN_INFO "Mailbox %d: Waking up everything", mailbox->owner);
    wake_up_locked(&mailbox->modify_queue);

    printk(KERN_INFO "Mailbox %d: Mailbox Stopped", mailbox->owner);

    mailbox_unlock(mailbox, &flags);
    return 0;
}

long mailbox_destroy(Mailbox* mailbox) {
    Message *msg, *next_msg;
    unsigned long flags;

    printk(KERN_INFO "Mailbox %d: Destroying, stopping", mailbox->owner);
    printk(KERN_INFO "Mailbox %d: Stopping mailbox with %d left waiting", mailbox->owner, atomic_read(&mailbox->waiting));
    mailbox_stop(mailbox);

    printk(KERN_INFO "Mailbox %d: Trying to get mailbox lock", mailbox->owner);
    
    
    mailbox_lock(mailbox, &flags);


    printk(KERN_INFO "Mailbox %d: Waiting until other processes finish with this mailbox", mailbox->owner);
    wait_event_interruptible_locked_irq(mailbox->modify_queue, atomic_read(&mailbox->waiting) == 0);

    printk(KERN_INFO "Mailbox %d: Flushing messages", mailbox->owner);

    list_for_each_entry_safe(msg, next_msg, &mailbox->messages, list) {
        list_del(&msg->list);
        message_destroy(&msg);
    }

    printk(KERN_INFO "Mailbox %d: No more messages, destroying mailbox", mailbox->owner);

    //mailbox_unlock(mailbox, &flags);
    kmem_cache_free(mailbox_cache, mailbox);
    
    return 0;
}
