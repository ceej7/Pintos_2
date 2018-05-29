// #include "userprog/syscall.h"
// #include <stdio.h>
// #include <syscall-nr.h>
// #include "threads/interrupt.h"
// #include "threads/thread.h"

// static void syscall_handler (struct intr_frame *);


//My implement
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

#define MAX_ARGS 3
#define USER_VADDR_BOTTOM ((void *) 0x08048000)

struct lock filesys_lock;

struct process_file {
  struct file *file;
  int fd;
  struct list_elem elem;
};

int process_add_file (struct file *f);
struct file* process_get_file (int fd);

static void syscall_handler (struct intr_frame *);
int user_to_kernel_ptr(const void *vaddr);
void get_arg (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
void check_valid_buffer (void* buffer, unsigned size);








void
syscall_init (void) 
{
	//test
	lock_init(&filesys_lock);


  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!\n");
  // thread_exit ();
  //My implement
  int arg[MAX_ARGS];
  check_valid_ptr((const void*) f->esp);  //check whether is usr's valid addr
  switch (* (int *) f->esp)               //switch to determine the type of intr
    {
    case SYS_HALT:
      {
        halt();
        break;
      }
    case SYS_EXIT:
      {
        get_arg(f, &arg[0], 1);
        exit(arg[0]);
        break;
      }
    case SYS_EXEC:
      {
        get_arg(f, &arg[0], 1);
        //check if all bytes within range are correct
        arg[0] = user_to_kernel_ptr((const void *) arg[0]);
        f->eax = exec((const char *) arg[0]);
        break;
      }
    case SYS_WAIT:
      {
        get_arg(f, &arg[0], 1);
        f->eax = wait(arg[0]);
        break;
      }
    case SYS_CREATE:
      {
        get_arg(f, &arg[0], 2);
        //check if all bytes within range are correct
        arg[0] = user_to_kernel_ptr((const void *) arg[0]);
        f->eax = create((const char *)arg[0], (unsigned) arg[1]);
        break;
      }
    case SYS_REMOVE:
      {
        get_arg(f, &arg[0], 1);
        arg[0] = user_to_kernel_ptr((const void *) arg[0]);
        f->eax = remove((const char *) arg[0]);
        break;
      }
    case SYS_OPEN:
      {
        get_arg(f, &arg[0], 1);
        arg[0] = user_to_kernel_ptr((const void *) arg[0]);
        f->eax = open((const char *) arg[0]);
        break;
      }
    case SYS_FILESIZE:
      {
        get_arg(f, &arg[0], 1);
        f->eax = filesize(arg[0]);
        break;
      }
    case SYS_READ:
      {
        get_arg(f, &arg[0], 3);
        //check the buffer for read if they are valid
        check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
        //check if all bytes within range are correct
        arg[1] = user_to_kernel_ptr((const void *) arg[1]);
        f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
        break;
      }
    case SYS_WRITE:
      {
        get_arg(f, &arg[0], 3);
        //check the buffer for write if they are valid
        check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
        //check if all bytes within range are correct
        arg[1] = user_to_kernel_ptr((const void *) arg[1]);
        f->eax = write(arg[0], (const void *) arg[1],
                   (unsigned) arg[2]);
        break;
      }
    case SYS_SEEK:
      {
        get_arg(f, &arg[0], 2);
        seek(arg[0], (unsigned) arg[1]);
        break;
      }
    case SYS_TELL:
      {
        get_arg(f, &arg[0], 1);
        f->eax = tell(arg[0]);
        break;
      }
    case SYS_CLOSE:
      {
        get_arg(f, &arg[0], 1);
        close(arg[0]);
        break;
      }
    }
}


//test

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{
  struct thread *cur = thread_current();
  //father is still waiting?
  if (thread_alive(cur->parent))
    {
      cur->cp->status = status;
    }
  printf ("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

pid_t exec (const char *cmd_line)
{
  //execute process and get its pid
  pid_t pid = process_execute(cmd_line);
  //get the cp of pid
  struct child_process* cp = get_child_process(pid);
  ASSERT(cp);
  //if cp->load is not NOT_LOAD status return ERROR
  while (cp->load == NOT_LOADED)
    {
      barrier();
    }
  if (cp->load == LOAD_FAIL)
    {
      return ERROR;
    }
  return pid;
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
  //current thread sleep until filesys-lock
  lock_acquire(&filesys_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return success;
}

bool remove (const char *file)
{
  lock_acquire(&filesys_lock);
  bool success = filesys_remove(file);
  lock_release(&filesys_lock);
  return success;
}

int open (const char *file)
{
  lock_acquire(&filesys_lock);
  //open the file
  struct file *f = filesys_open(file);
  //Error resolution
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  //add file to current-process
  int fd = process_add_file(f);
  lock_release(&filesys_lock);
  return fd;
}

int filesize (int fd)
{
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  int size = file_length(f);
  lock_release(&filesys_lock);
  return size;
}

int read (int fd, void *buffer, unsigned size)
{
  //fd is fileno, read from input
  if (fd == STDIN_FILENO)
    {
      unsigned i;
      uint8_t* local_buffer = (uint8_t *) buffer;
      for (i = 0; i < size; i++)
    	{
        //retrieve bytes for input buffer
    	  local_buffer[i] = input_getc();
    	}
      return size;
    }
  //make sure mutex
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  //error solution
  if (!f)
  {
    lock_release(&filesys_lock);
    return ERROR;
  }
  //do by file_read
  int bytes = file_read(f, buffer, size);
  lock_release(&filesys_lock);
  return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
  //fd is fileno, write to input
  if (fd == STDOUT_FILENO)
    {
      putbuf(buffer, size);
      return size;
    }
  //promise mutex
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  //error solution
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
    //write into buffer
  int bytes = file_write(f, buffer, size);
  lock_release(&filesys_lock);
  return bytes;
}

void seek (int fd, unsigned position)
{
  //promise mutex
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return;
    }
  //use file_seek
  file_seek(f, position);
  lock_release(&filesys_lock);
}

unsigned tell (int fd)
{
  //promise mutex
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  //user file_tell
  off_t offset = file_tell(f);
  lock_release(&filesys_lock);
  return offset;
}

void close (int fd)
{
  lock_acquire(&filesys_lock);
  process_close_file(fd);
  lock_release(&filesys_lock);
}

void check_valid_ptr (const void *vaddr)
{
  if (!is_user_vaddr(vaddr) || vaddr < USER_VADDR_BOTTOM)
    {
      exit(ERROR);
    }
}

int user_to_kernel_ptr(const void *vaddr)
{
  // TO DO: Need to check if all bytes within range are correct
  // for strings + buffers
  check_valid_ptr(vaddr);
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr)
    {
      exit(ERROR);
    }
  return (int) ptr;
}

int process_add_file (struct file *f)
{
  //init struct process_file 
  struct process_file *pf = malloc(sizeof(struct process_file));
  pf->file = f;
  pf->fd = thread_current()->fd;

  //thread-current's increase for cnt
  thread_current()->fd++;
  //add to current-thread's file-list
  list_push_back(&thread_current()->file_list, &pf->elem);
  return pf->fd;
}

struct file* process_get_file (int fd)
{
  struct thread *t = thread_current();
  struct list_elem *e;
  //thread-current's traversal to get a file which fd is equal to the param 
  for (e = list_begin (&t->file_list); e != list_end (&t->file_list);
       e = list_next (e))
        {
          struct process_file *pf = list_entry (e, struct process_file, elem);
          if (fd == pf->fd)
	    {
	      return pf->file;
	    }
        }
  return NULL;
}

void process_close_file (int fd)
{
  struct thread *t = thread_current();
  struct list_elem *next, *e = list_begin(&t->file_list);
  //traverse the whole file-list of current thread
  while (e != list_end (&t->file_list))
    {
      next = list_next(e);
      struct process_file *pf = list_entry (e, struct process_file, elem);
      //if file of this fd  =param || the file of this fs is in CLOSE_ALL
      if (fd == pf->fd || fd == CLOSE_ALL)
    	{
    	  file_close(pf->file); //close file
    	  list_remove(&pf->elem);//remove from list
    	  free(pf);
    	  if (fd != CLOSE_ALL)
    	    {
    	      return;
    	    }
    	}
      e = next;
    }
}

struct child_process* add_child_process (int pid)
{
	//init the cp of the thread to create and add it into father's child list
  struct child_process* cp = malloc(sizeof(struct child_process));
  cp->pid = pid;
  cp->load = NOT_LOADED;
  cp->wait = false;
  cp->exit = false;
  //lock_init(&cp->wait_lock);
  list_push_back(&thread_current()->child_list,
		 &cp->elem);
  return cp;
}

struct child_process* get_child_process (int pid)
{
  //traversal the child_list of current thread to find the child with pid 
  struct thread *t = thread_current();
  struct list_elem *e;

  for (e = list_begin (&t->child_list); e != list_end (&t->child_list);
       e = list_next (e))
        {
          struct child_process *cp = list_entry (e, struct child_process, elem);
          if (pid == cp->pid)
		    {
		      return cp;
		    }
      	}
  return NULL;
}

void remove_child_process (struct child_process *cp)
{
	//remove cp from its list
  list_remove(&cp->elem);
  free(cp);
}

void remove_child_processes (void)
{
	//remove all child from current thread 
  struct thread *t = thread_current();
  struct list_elem *next, *e = list_begin(&t->child_list);

  while (e != list_end (&t->child_list))
    {
      next = list_next(e);
      struct child_process *cp = list_entry (e, struct child_process,
					     elem);
      list_remove(&cp->elem);
      free(cp);
      e = next;
    }
}

//get n args from *f and store it in *arg
void get_arg (struct intr_frame *f, int *arg, int n)
{

  int i;
  int *ptr;
  //extract n args from 1~n shifting overf->esp
  for (i = 0; i < n; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_valid_ptr((const void *) ptr);
      arg[i] = *ptr;
    }
}

void check_valid_buffer (void* buffer, unsigned size)
{
  unsigned i;
  char* local_buffer = (char *) buffer;
  for (i = 0; i < size; i++)
    {
      check_valid_ptr((const void*) local_buffer);
      local_buffer++;
    }
}




