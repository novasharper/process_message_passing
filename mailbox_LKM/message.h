#ifndef __MESSAGE__
#define __MESSAGE__

#define MAX_MSG_SIZE 128

#define MSG_LENGTH_ERROR    1005
#define MSG_ARG_ERROR       1006

typedef struct __message {
    pid_t sender;               // Record the sender in the message
    int len;                    // The length of the message, cannot exceed MAX_MESSAGE_SIZE
    char msg[MAX_MSG_SIZE]; // The message itself
    struct list_head list;      // Linked list
} Message;

void message_init(void);
void message_exit(void);

long message_create(Message** message, pid_t sender, void* msg, int len);
long message_destroy(Message** message);

#endif