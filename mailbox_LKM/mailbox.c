/**
 * Pat Long
 * Mar 1, 2014
 * CS 3013
 * Mailbox LKM
 * Manage mailboxes and sending/recieving messages
 */

// We need to define __KERNEL__ and MODULE to be in Kernel space
// If they are defined, undefined them and define them again:
#undef __KERNEL__
#undef MODULE

#define __KERNEL__ 
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

#include "mailbox_impl.h"

unsigned long **sys_call_table;

asmlinkage long (*ref_sys_cs3013_syscall1)(void);
asmlinkage long (*ref_sys_cs3013_syscall2)(void);
asmlinkage long (*ref_sys_cs3013_syscall3)(void);
asmlinkage long (*ref_sys_exit)(int status);
asmlinkage long (*ref_sys_exit_group)(int status);
int mailbox_cleanup_proc = -1;
int mailbox_cleanup_finished = 0;

static int __process_is_valid(pid_t process) {
	struct task_struct* task = pid_task(find_get_pid(process), PIDTYPE_PID);
	// Task exists
	if (task) {
		// Task isn't exiting or unrunnable
		if (!task->state) {
			return 1;
		}
	}
	return 0;
}
// Send message to destination process with given message and whether or not to block until sent
asmlinkage long __send_message(pid_t dest, void *msg, int len, bool block) {
	pid_t current_pid = current->pid;
	Message* message_s;
	unsigned long spin_lock_flags;

	if (__process_is_valid(dest)) {
		Mailbox* mailbox = get_create_mailbox(dest);
		if (mailbox->stopped) {
			return MAILBOX_STOPPED;
		} else {
			// Got mailbox successfully, now do checking for message
			if (len > MAX_MSG_SIZE) {
				return MSG_LENGTH_ERROR;
			} else {
				__init_message(&message_s);
				if (copy_from_user(&message_s->msg, msg, len)) {
					// Failed to create message, destroy it.
					__destroy_message(&message_s);
					return MSG_ARG_ERROR;
				} else {
					message_s->len = len;
					message_s->sender = current_pid;

					// Get spin lock for mailbox modification
					spin_lock_irqsave(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);

					// Check if mailbox is full
					if (mailbox->message_count >= mailbox->message_max) {

						// If the mailbox is full, either block and wait, or return error
						if (block) {
							// Wait until mailbox isn't full, or mailbox is stopped
							wait_event_interruptible_exclusive_locked_irq(mailbox->send_recieve_message_queue, (mailbox->stopped || mailbox->message_count < mailbox->message_max));
							if (mailbox->stopped) {
								spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
								return MAILBOX_STOPPED;
							}
						} else {
							spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
							return MAILBOX_FULL;
						}
					}

					// Mailbox isn't full, add the message
					__mailbox_add_message_unsafe(mailbox, message_s);
					spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
				}
			}
		}
	} else {
		// Process is not valid
		return MAILBOX_INVALID;
	}

	return 0;
}

// Recieve message from mailbox with its sender
asmlinkage long __recieve_message(pid_t *sender, void *msg, int *len, bool block) {
	pid_t current_pid = current->pid;
	unsigned long spin_lock_flags;

	// The process is def alive if it's calling a syscall, no need for validtation here.
	Mailbox* mailbox = get_create_mailbox(current_pid);
	Message* message;


	// Aquire lock, we're reading and writing
	spin_lock_irqsave(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
	// Do we not have messages? - either wait or return
	if (mailbox->message_count == 0) {
		// Are we stopped?
		if (mailbox->stopped) {
			// We ain't waiting for no stopped mailbox
			spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
			return MAILBOX_STOPPED;
		} else {
			// Oh, I guess it's not stopped, well are we waiting at least?
			if (block) {
				// Oh we're watching, and we're waiting
				wait_event_interruptible_exclusive_locked_irq(mailbox->send_recieve_message_queue, ((mailbox->stopped && mailbox->message_count == 0) || mailbox->message_count > 0));
				if (mailbox->stopped) {
					// No more messages, and the mailbox stopped
					spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
					return MAILBOX_STOPPED;
				}
			} else {
				spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
				return MAILBOX_EMPTY;
			}
		}
	}

	// At this point, we have messages, or waited until we have messages
	// Get message code
	message = list_entry(&mailbox->messages, Message, list);
	// Copy 
	copy_to_user(&(message->sender), sender, sizeof(pid_t));
	copy_to_user(&(message->len), len, sizeof(int));
	copy_to_user(&(message->msg), msg, message->len);
	__mailbox_remove_message_unsafe(mailbox, message);
	__destroy_message(&message);
	spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, spin_lock_flags);
	return 0;
}

asmlinkage long __manage_mailbox(bool stop, int *count) {
	pid_t current_pid = current->pid;
	unsigned long flags;

	// The process is def alive if it's calling a syscall, no need for validtation here.
	Mailbox* mailbox = get_create_mailbox(current_pid);
	if(stop) {
		// Grab the message queue spin lock, so we don't stop in the middle of a sendmessage attempt
		// (prevents race condition of stopping mailbox after send_msg has already checked mailbox->stopped)
		// 
		// After we stop the mailbox we must wake up everything in the queue
		spin_lock_irqsave(&mailbox->send_recieve_message_queue.lock, flags);
		__mailbox_stop_unsafe(mailbox);
		wake_up_locked(&mailbox->send_recieve_message_queue);
		spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, flags);
	}
	copy_to_user(&(mailbox->message_count), count, sizeof(int));
	return 0;
}

asmlinkage long __new_sys_exit(int status) {
	// Must check if thread group is dead, if so, delete mailbox
	return ref_sys_exit(status);
}

asmlinkage long __new_sys_exit_group(int status) {
	pid_t current_pid = current->pid;
	unsigned long flags;
	// The process is def alive if it's calling a syscall, no need for validtation here.
	Mailbox* mailbox = get_create_mailbox(current_pid);

	// Stop the mailbox
	spin_lock_irqsave(&mailbox->send_recieve_message_queue.lock, flags);
	__mailbox_stop_unsafe(mailbox);

	// This will wake up everything, since everything that waits will continue if stopped is true
	wake_up_locked(&mailbox->send_recieve_message_queue);

	// Wait until all messages are gone
	wait_event_interruptible_exclusive_locked_irq(mailbox->send_recieve_message_queue, mailbox->message_count == 0);


	spin_unlock_irqrestore(&mailbox->send_recieve_message_queue.lock, flags);

	// outside of the spin lock as the lock gets destroyed when we destroy the mailbox.
	destroy_mailbox_unsafe(mailbox);

	return ref_sys_exit_group(status);
}

static unsigned long **find_sys_call_table(void) {
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

		if (sct[__NR_close] == (unsigned long *) sys_close) {
			printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX", (unsigned long) sct);
			return sct;
		}

	offset += sizeof(void *);
	}

	return NULL;
}	// static unsigned long **find_sys_call_table(void)

static void disable_page_protection(void) {
	/*
	Control Register 0 (cr0) governs how the CPU operates.

	Bit #16, if set, prevents the CPU from writing to memory marked as
	read only. Well, our system call table meets that description.
	But, we can simply turn off this bit in cr0 to allow us to make
	changes. We read in the current value of the register (32 or 64
	bits wide), and AND that with a value where all bits are 0 except
	the 16th bit (using a negation operation), causing the write_cr0
	value to have the 16th bit cleared (with all other bits staying
	the same. We will thus be able to write to the protected memory.

	It's good to be the kernel!
	*/

	write_cr0 (read_cr0 () & (~ 0x10000));

}	//static void disable_page_protection(void)


static void enable_page_protection(void) {
	/*
	See the above description for cr0. Here, we use an OR to set the
	16th bit to re-enable write protection on the CPU.
	*/

	write_cr0 (read_cr0 () | 0x10000);

}	// static void enable_page_protection(void)

static int __init interceptor_start(void) {
	/* Find the system call table */
	if(!(sys_call_table = find_sys_call_table())) {
		/* Well, that didn't work.
		Cancel the module loading step. */
		return -1;
	}

	// Initialize the mailbox implementation
	mailbox_impl_init();

	/* Store a copy of all the existing functions */
	ref_sys_cs3013_syscall1 = (void *)sys_call_table[__NR_cs3013_syscall1];
	ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];
	ref_sys_cs3013_syscall3 = (void *)sys_call_table[__NR_cs3013_syscall3];
	ref_sys_exit = (void *)sys_call_table[__NR_exit];
	ref_sys_exit_group = (void *)sys_call_table[__NR_exit_group];

	/* Replace the existing system calls */
	disable_page_protection();

	sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)__send_message;
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)__recieve_message;
	sys_call_table[__NR_cs3013_syscall3] = (unsigned long *)__manage_mailbox;
	sys_call_table[__NR_exit] = (unsigned long *)__new_sys_exit;
	sys_call_table[__NR_exit_group] = (unsigned long *)__new_sys_exit_group;

	enable_page_protection();

	/* And indicate the load was successful */
	printk(KERN_INFO "Loaded interceptor!");

	return 0;
}	// static int __init interceptor_start(void)


static void __exit interceptor_end(void) {
	/* If we don't know what the syscall table is, don't bother. */
	if(!sys_call_table)
		return;

	/* Revert all system calls to what they were before we began. */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)ref_sys_cs3013_syscall1;
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
	sys_call_table[__NR_cs3013_syscall3] = (unsigned long *)ref_sys_cs3013_syscall3;
	sys_call_table[__NR_exit] = (unsigned long *)ref_sys_exit;
	sys_call_table[__NR_exit_group] = (unsigned long *)ref_sys_exit_group;
	enable_page_protection();

	// Clean up our mailbox
	mailbox_impl_exit();

	printk(KERN_INFO "Unloaded interceptor!");
}	// static void __exit interceptor_end(void)

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);