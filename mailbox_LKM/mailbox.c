/**
 * Pat Long
 * Feb 21, 2014
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

#include "mailbox_impl.h"

unsigned long **sys_call_table;

asmlinkage long (*ref_sys_cs3013_syscall1)(void);
asmlinkage long (*ref_sys_cs3013_syscall2)(void);
asmlinkage long (*ref_sys_cs3013_syscall3)(void);
asmlinkage long (*ref_sys_exit)(int status);
asmlinkage long (*ref_sys_exit_group)(int status);
int mailbox_cleanup_proc = -1;
int mailbox_cleanup_finished = 0;

// Send message to destination process with given message and whether or not to block until sent
asmlinkage long __send_message(pid_t dest, void *msg, int len, bool block) {
	/**
	 * first, 
	 */

	return 0;
}

// Recieve message from mailbox with its sender
asmlinkage long __recieve_message(pid_t *sender, void *msg, int *len, bool block) {
	return 0;
}

asmlinkage long __manage_mailbox(bool stop, int *count) {
	return 0;
}

asmlinkage long __new_sys_exit(int status) {
	// Must check if thread group is dead, if so, delete mailbox
	return ref_sys_exit(status);
}

asmlinkage long __new_sys_exit_group(int status) {
	// Delete the mailbox, bro
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