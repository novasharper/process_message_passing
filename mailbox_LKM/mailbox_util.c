#include "mailbox_util.h"

// kmem_cache_t* kmem_cache_create(const char* name, size_t size, size_t align, unsigned long flags, NULL)
// void* kmem_cache_alloc(kmem_cache_t* cachep, int flags)
// void kmem_cache_free(kmem_cache_t* cachep, void* objp)
// int kmem_cache_destroy(kmem_cache_t* cachep)

void init_mailboxes(void) {
	mailboxes = ht_init(128, NULL);
	init_waitqueue_head(&stop_event);
}

Mailbox *create_mailbox(pid_t proc_id) {
	// Create mailbox
	Mailbox *mailbox = (Mailbox *) kmalloc(sizeof(Mailbox), GFP_KERNEL)
	// Create message chache
	mailbox->mem_cache = kmem_cache_create("mailbox_messages", sieof(Message), 0, 0, NULL);
	INIT_LIST_HEAD(&(mailbox->message_list.list));
	// Add to hash table
	/** TODO: Synchronization crap **/
	ht_insert(mailboxes, &proc_id, sizeof(pid_t), mailbox, sizeof(Mailbox));

	return mailbox;
}

long create_message(pid_t proc_id, Message *new_message) {
	Mailbox *mailbox;
	if(get_mailbox(proc_id, mailbox) == MAILBOX_STOPPED) return MAILBOX_STOPPED;
	new_message = (Message *) kmem_cache_alloc(mailbox->mem_cache, GFP_KERNEL);
	new_message->next = NULL;
}

long append_message(pid_t proc_id, Message *message) {
	Mailbox *mailbox;
	get_mailbox(proc_id, mailbox);
	
	wait_queue_t wait;

	init_waitqueue_entry(&wait, current);
	current->state = TASK_INTERRUPTIBLE;

	add_wait_queue(mailbox->write_queue, &wait);
	
	if (mailbox->stopped) {
		remove_wait_queue(mailbox->write_queue, &wait);
		return MAILBOX_STOPPED;
	}

	list_add(&(message->list), &(mailbox->message_list.list));
	message_count++;

	remove_wait_queue(mailbox->write_queue, &wait);
	return 0;
}

long get_message(pid_t proc_id, Message *message) {
	Mailbox *mailbox;
	get_mailbox(proc_id, mailbox);
	
	wait_queue_t wait;

	init_waitqueue_entry(&wait, current);
	current->state = TASK_INTERRUPTIBLE;

	// Wait until there are messages
	while(mailbox->message_count == 0) {
		if (mailbox->stopped) return MAILBOX_STOPPED;
		usleep(1);
	}

	add_wait_queue(mailbox->read_queue, &wait);
	if(mailbox->stopped) {
		remove_wait_queue(mailbox->read_queue, &wait);
		return MAILBOX_STOPPED;
	}

	Message *next = list_next_entry(&(mailbox->head), list);
	Message *message = &(mailbox->head);
	&(mailbox->head) = next;
	list_del(message);
	message_count--;

	remove_wait_queue(mailbox->read_queue, &wait);
	return 0;
}

void destroy_message(pid_t proc_id, Message *to_delete) {
	Mailbox *mailbox = get_mailbox(proc_id);
	kmem_cache_free(mailbox->mem_cache, to_delete);
}

void destroy_mailbox(pid_t proc_id) {
	// Make sure mailbox is completely free
	Mailbox *to_delete = get_mailbox(proc_id);
	// Safe to delete
	if(to_delete->stopped && to_delete->message_count == 0) {
		// Remove mailbox from hash table
		ht_remove(mailboxes, &proc_id, sizeof(pid_t));
		// Clear slab
		kmem_cache_destroy(to_delete->mem_cache);
		// Free memory space
		kfree(to_delete);
	}
}

// Get mailbox by process ID
long get_mailbox(pid_t proc_id, Mailbox *mailbox) {
	mailbox = (Mailbox *) ht_search(mailboxes, &proc_id, sizeof(pid_t));
	if(mailbox != NULL && mailbox->stopped) return MAILBOX_STOPPED;
	if(mailbox == NULL) mailbox = create_mailbox(proc_id);
	return 0;
}