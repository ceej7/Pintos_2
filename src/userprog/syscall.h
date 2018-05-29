#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

#define CLOSE_ALL -1
#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2


//My implement
struct child_process {
  int pid;               //process id
  int load;				 //load status
  bool wait;			 //is wait?
  bool exit;			 //is exit?
  int status;			 //exit status
  //struct lock wait_lock; //for wait lock
  struct list_elem elem; //position in child_list of father process
};


//My implement
struct child_process* add_child_process (int pid);
struct child_process* get_child_process (int pid);
void remove_child_process (struct child_process *cp);
void remove_child_processes (void);


void syscall_init (void);


//test
void process_close_file (int fd);

#endif /* userprog/syscall.h */
