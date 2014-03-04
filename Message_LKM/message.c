/**
 * @file message.c
 */

// We're in kernel space
#undef __KERNEL__
#undef MODULE

#define __KERNEL__ 
#define MODULE

#include <linux/syscalls.h>
#include <linux/slab.h>

#include "message.h"

static struct kmem_cache* message_cache;

void message_init() {
    message_cache = kmem_cache_create("message", sizeof(Message), 0, 0, NULL);
}

void message_exit() {
    kmem_cache_destroy(message_cache);
}

/**
 * Creates a message, doesn't add it anywhere
 * @param  message [description]
 * @param  sender  [description]
 * @param  msg     [description]
 * @param  len     [description]
 * @return         [description]
 */
long message_create(Message** message, pid_t sender, void* msg, int len) {
    if (len > MAX_MSG_SIZE || len < 0) {
        return MSG_LENGTH_ERROR;
    }

    *message = kmem_cache_alloc(message_cache, 0);

    INIT_LIST_HEAD(&(*message)->list);

    (*message)->sender = sender;
    (*message)->len = len;
    if (copy_from_user(&(*message)->msg, msg, len)) {
        kmem_cache_free(message_cache, *message);
        *message = NULL;
        return MSG_ARG_ERROR;
    } else {
        return 0;
    }
}

/**
 * Destroys a message, doesn't remove it from anywhere (if it's in a linked list, you must remove it first)
 * @param  message [description]
 * @return         [description]
 */
long message_destroy(Message** message) {
    printk(KERN_INFO "Destroying message");
    kmem_cache_free(message_cache, *message);
    *message = NULL;
    return 0;
}
