/*
 * pidlist.c
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
//#include <pthread.h>
#include <semaphore.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pidlist.h"

#define DEBUG  false

//#define LOCK    pthread_mutex_lock
//#define UNLOCK  pthread_mutex_unlock
#define LOCK    sem_wait
#define UNLOCK  sem_post

#define MAX_PIDS 100
#define MAX_NAME_LENGTH  128

typedef struct pid_list pid_list_handle_t;

struct pid_list {
	struct pid_node *head;
	struct pid_node *tail;
	int count;
	char name[MAX_NAME_LENGTH];
};

typedef struct pid_node {
    struct pid_node *next;
    struct pid_node *prev;
	int index;
	pid_t pid;
} pid_node_t;

struct pidlist_region {
	struct pid_node node[MAX_PIDS];
	struct pid_list free;
	struct pid_list active;
	//pthread_mutex_t lock;
	sem_t lock;
};

static struct pidlist_region *rpid = NULL;
static int fd = -1;

#define  active   rpid->active
#define  freelist rpid->free

/*
 *  random_max
 *
 *  Generates a random number between 1 and max
 *
 *  Inputs:
 *    max - the maximum number to generate
 *
 *  Returns:
 *    int - the randomly generated number
 */
static int random_max( int max ) {
    int val = (int)((double)max*(double)rand()/((double)RAND_MAX+1.0));
    return val;
}

/*
 *  initialize_pid_node
 *
 *  Initializes a pid_node_t object
 *
 *  Inputs:
 *    node - the node to initialize
 *    i - the index of the node
 *
 *  Returns: --
 */
static void initialize_pid_node( pid_node_t * node, int i ) {
	node->next = NULL;
	node->prev = NULL;
	node->index = i;
	node->pid = -1;
}

/*
 *  initialize_pid_list
 *
 *  Initializes the PID linked list
 *
 *  Inputs:
 *    list - the linked list to initialize
 *    listname - a string name for the list
 *
 *  Returns: --
 */
static void initialize_pid_list( pid_list_handle_t *list, const char * listname ) {

	list->head = NULL;
	list->tail = NULL;
	list->count = 0;
	strcpy( list->name, listname );

}

/*
 *  pidlist_print
 *
 *  Prints the entire list to the console; useful during debug.
 *
 *  Inputs:
 *    list - the PID linked list
 *
 *  Returns: --
 */
static void pidlist_print( pid_list_handle_t *list ) {

	struct pid_node *node;

	printf("%s [%d]: ",list->name,list->count);

	node = list->head;
	while (node != NULL) {
		printf(" %d[%d]",node->index,node->pid);
		node = node->next;
	}
	printf("\n");
}

/*
 *  remove_from_list
 *
 *  Removes the specified node from the linked list.
 *
 *  Note: it is up to the caller to ensure the node exists within
 *  the list.
 *
 *  Inputs:
 *    list - the PID linked list
 *    node - the node to remove
 *
 *  Returns: --
 */
static void remove_from_list( pid_list_handle_t *list, pid_node_t *node ) {

	if (node == list->head && node == list->tail) {
		// Only one node
		list->head = NULL;
		list->tail = NULL;
	} else if (node == list->head && node != list->tail) {
		// first node - update list->head
		list->head = list->head->next;
		list->head->prev = NULL;
	} else if (node != list->head && node == list->tail) {
		// last node - update list->tail
		list->tail = list->tail->prev;
		list->tail->next = NULL;
	} else {  // node != list->head && node != list->tail
		// middle node
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}

	// remove linkage
	node->next = NULL;
	node->prev = NULL;

	list->count--;

	if (DEBUG) pidlist_print( list );

}

/*
 *  add_to_tail
 *
 *  Adds the specified node to the tail of the PID linked list
 *
 *  Inputs:
 *    list - the PID linked list
 *    node - the node to be appended
 *
 *  Returns: --
 */
static inline void add_to_tail( pid_list_handle_t *list, pid_node_t *node ) {

	if (list->head == NULL) {
		list->head = node;
		list->tail = node;
	} else {
		node->prev = list->tail;
		list->tail->next = node;
		list->tail = node;
	}

	list->count++;

	if (DEBUG) pidlist_print( list );
}

/*
 *  remove_from_head
 *
 *  Removes the nead node from the list
 *
 *  Inputs:
 *    list - the PID linked list
 *
 *  Returns:
 *    pid_node_t * - the head node pointer
 */
static inline pid_node_t * remove_from_head( pid_list_handle_t *list ) {

	pid_node_t *node = NULL;

	if (list->head == NULL) {
		return NULL;
	}

	node = list->head;

	remove_from_list( list, node );

	return node;
}

/*
 *   find_node
 *
 *   Adds the specified PID to the linked list
 *
 *  Inputs:
 *    pid - the PID to add
 *
 *  Return:
 *    0 on success; -1 on failure
 */
static pid_node_t * find_node( pid_list_handle_t *list, pid_t pid ) {

	pid_node_t * node = NULL;

	// find the node with the specified pid
	node = list->head;
	while (node != NULL && node->pid != pid) {
		node = node->next;
	}

	return node;
}

/*
 *   pidlist_add_pid
 *
 *   Adds the specified PID to the linked list
 *
 *  Inputs:
 *    pid - the PID to add
 *
 *  Return:
 *    0 on success; -1 on failure
 */
int pidlist_add_pid( pid_t pid ) {

	pid_node_t *node = NULL;

	LOCK( &rpid->lock );

	node = find_node( &active, pid );
	if (node != NULL) {
		if (DEBUG) printf("[%u] ADD_PID %u pid already exists in the list\n",getpid(), pid);
		UNLOCK( &rpid->lock );
		return 0;		// idempotent
	}

	node = remove_from_head( &freelist );

	if (node == NULL) {
		if (DEBUG) printf("[%u] ADD_PID %u failed, no space\n",getpid(), pid);
		UNLOCK( &rpid->lock );
		return -1;		// no space
	}

	// set pid
	node->pid = pid;

	printf("[%u] ADD_PID %u\n",getpid(), pid);

	add_to_tail( &active, node );

	UNLOCK( &rpid->lock );

	return 0;		// add successful
}

/*
 *   pidlist_remove_pid
 *
 *   Removes the specified PID from the linked list
 *
 *  Inputs:
 *    pid - the PID to remove
 *
 *  Return:
 *    --
 */
void pidlist_remove_pid( pid_t pid ) {

	struct pid_node *node;

	LOCK( &rpid->lock );

	node = find_node( &active, pid );
	if (node == NULL) {
		if (DEBUG) printf("[%u] REMOVE_PID %u failed - pid not found\n",getpid(), pid);
		UNLOCK( &rpid->lock );
		return;
	}

	printf("[%u] REMOVE_PID %u\n",getpid(), pid);

	remove_from_list( &active, node );

	// clear pid
	node->pid = -1;

	add_to_tail( &freelist, node );

	UNLOCK( &rpid->lock );

}


/*
 *   pidlist_get_count
 *
 *   Returns the count of pids in the list
 *
 *  Inputs: --
 *  Return:
 *     pid_t - the pid object
 */
int pidlist_get_count( void ) {
	return active.count;
}


/*
 *   pidlist_get_random
 *
 *   Retrieves a random pid from the list of active pids
 *
 *  Inputs: --
 *  Return:
 *     pid_t - the pid object
 */
pid_t pidlist_get_random( void ) {

	int i, index;
	pid_node_t * node = NULL;
	pid_t pid;

	LOCK( &rpid->lock );

	index = random_max( active.count );

	i=0;
	node = active.head;
	while (node != NULL && i < index) {
		node = node->next;
		i++;
	}

	if (node == NULL) {
		if (DEBUG) printf("ERROR: get_random returned NULL pointer!\n");
		pid = -1;
	} else {
		pid = node->pid;
	}

	UNLOCK( &rpid->lock );

	return pid;
}

/*
 *  pidlist_init
 *
 *  Return the PID at the head of the list.
 *
 *  Inputs: --
 *  Return:
 *     pid_t - the pid object
 */
pid_t pidlist_get_first( void ) {

	pid_t pid;

	LOCK( &rpid->lock );

	pid = active.head->pid;

	UNLOCK( &rpid->lock );

	return pid;
}

/*
 *  pidlist_init
 *
 *  Initialize the shared memory PID linked list manager.
 *
 *  Inputs: --
 *  Return: --
 */
int pidlist_init( void ) {

	int i, rc;

	if (rpid == NULL) {

		printf("PID list manager initialized\n");

		fd = shm_open("/pidregion", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if (fd < 0) {
			return -1;
		}

		if (ftruncate(fd, sizeof(struct pidlist_region)) == -1) {
			/* Handle error */;
			return -1;
		}

		/* Map shared memory object */
		rpid = mmap(NULL, sizeof(struct pidlist_region),
			   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (rpid == MAP_FAILED) {
			/* Handle error */;
			return -1;
		}

		//rc = pthread_mutex_init( &rpid->lock, NULL );
		rc = sem_init( &rpid->lock, 1, 1);
		if (rc != 0) {
			printf("sem init failure\n");
			return -1;
		}

		LOCK( &rpid->lock );

		initialize_pid_list( &freelist, "PID Free List" );
		initialize_pid_list( &active, "PID Active List" );

		// initialize all nodes and add to the free list
		for (i=0; i<MAX_PIDS; i++) {
			initialize_pid_node( &rpid->node[i], i );
			add_to_tail( &freelist, &rpid->node[i] );
		}

		UNLOCK( &rpid->lock );

	}

	return 0;
}


void print_lock_info( void ) {
	printf("[%u] File Descriptor is %d, shmem=0x%p\n",getpid(),fd,rpid);
}
