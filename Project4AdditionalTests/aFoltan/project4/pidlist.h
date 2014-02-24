/*
 * pidlist.h
 *
 *  Created on: Dec 11, 2011
 *      Author: Andrew Foltan
 *      CS-502_Cisco
 *      Project4
 */

#ifndef PIDS_H_
#define PIDS_H_

/*
 *  Function prototypes for managing the PID list
 */
extern int pidlist_init( void );
extern int pidlist_add_pid( pid_t pid );
extern void pidlist_remove_pid( pid_t pid );
extern int pidlist_get_count( void );
extern pid_t pidlist_get_random( void );
extern pid_t pidlist_get_first( void );
extern void print_lock_info( void );

#endif /* PIDS_H_ */
