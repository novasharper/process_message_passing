#include "mailbox_util.h"

// kmem_cache_t* kmem_cache_create(const char* name, size_t size, size_t align, unsigned long flags, NULL)
// void* kmem_cache_alloc(kmem_cache_t* cachep, int flags)
// void kmem_cache_free(kmem_cache_t* cachep, void* objp)
// int kmem_cache_destroy(kmem_cache_t* cachep)

void init_mailboxes(void) {
	mailboxes = ht_init(32, NULL);
}

void create_mailbox(pid_t proc_id) {
	// Create mailbox
	Mailbox *mailbox = (Mailbox *) kmalloc(sizeof(Mailbox), GFP_KERNEL)
	// Create message chache
	mailbox->mem_cache = kmem_cache_create("mailbox_messages", 12 + MAX_MSG_SIZE, 0, 0, NULL);
	// Add to hash table
	/** TODO: Synchronization crap **/
	ht_insert(mailboxes, &proc_id, sizeof(pid_t), mailbox, sizeof(Mailbox));
}

Message *create_message(pid_t proc_id) {
	Mailbox *mailbox = get_mailbox(proc_id);
	Message *new_message = (Message *) kmem_cache_alloc(mailbox->mem_cache, GFP_KERNEL);
	return new_message;
}

void destoroy_message(pid_t proc_id, Message *to_delete) {
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
Mailbox *get_mailbox(pid_t proc_id) {
	Mailbox *mailbox = (Mailbox *) ht_search(mailboxes, &proc_id, sizeof(pid_t));
	if(mailbox != NULL && !mailbox->stopped) return mailbox;
	return NULL;
}