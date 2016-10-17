#include <stdio.h>
#include <syscall-nr.h>
#include "process.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/off_t.h"
#include "devices/input.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"

#define EXECUTABLE_START (void *)0x08048000

struct lock file_sys_lock;
struct process_file
{
  struct file* file;
  struct lock file_lock;
  int file_descriptor;
  struct list_elem elem;
};

static void syscall_handler (struct intr_frame *);

typedef int pid_t;

static void sys_halt(void);
static pid_t sys_exec(const char* cmd_line);
static int sys_wait(pid_t pid);
static bool sys_create(const char* file, unsigned initial_size);
static bool sys_remove(const char* file);
static int sys_open(const char* file);
static int sys_filesize(int fd);
static int sys_read(int fd, void* buffer, unsigned size);
static int sys_write(int fd, const void* buffer, unsigned size);
static void sys_seek(int fd, unsigned position);
static unsigned sys_tell(int fd);
static void sys_close(int fd);


static void * create_kernel_ptr(const void* ptr);
static void get_args(struct intr_frame *f, int* args, int n);
static bool is_valid_ptr(const void* ptr);

void
syscall_init (void) 
{
  lock_init(&file_sys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int args[3];
  int syscall_no = *(int *)create_kernel_ptr(f->esp);  

  switch (syscall_no)
  {
    case SYS_HALT:
      sys_halt();
      break;
    case SYS_EXIT:
      get_args(f, args, 1);
      sys_exit(args[0]);
      break;
    case SYS_EXEC:
      get_args(f, args, 1);
      f->eax = sys_exec((const char *)args[0]);
      break;
    case SYS_WAIT:
      get_args(f, args, 1);
      f->eax = sys_wait(args[0]);
      break;
    case SYS_CREATE:
      get_args(f, args, 2);
      f->eax = sys_create((const char *)args[0], (unsigned) args[1]);
      break;
    case SYS_REMOVE:
      get_args(f, args, 1);
      f->eax = sys_remove((const char *) args[0]);
      break;
    case SYS_OPEN:
      get_args(f, &args[0], 1);
      f->eax = sys_open((const char *)args[0]);
      break;
    case SYS_FILESIZE:
      get_args(f, args, 1);
      f->eax = sys_filesize(args[0]);
      break;
    case SYS_READ:
      get_args(f, args, 3);
      f->eax = sys_read(args[0], (void *)args[1], (unsigned)args[2]);
      break;
    case SYS_WRITE:
      get_args(f, args, 3);
      f->eax = sys_write(args[0], (const void *)args[1], (unsigned)args[2]);
      break;
    case SYS_SEEK:
      get_args(f, args, 2);
      sys_seek(args[0], args[1]);    
      break;
    case SYS_TELL:
      get_args(f, args, 1);
      f->eax = sys_tell(args[0]);
      break;
    case SYS_CLOSE:
      get_args(f, args, 1);
      sys_close(args[0]);
      break;
    default:
      sys_exit(-1);
  }
}

/* Retrieve args from stack */
static void get_args(struct intr_frame *f, int* args, int n)
{
  // Stack pointer is pointing at the syscall number (int), args start just past that
  int * stack_args = f->esp + sizeof(int);

  for (int i = 0; i < n; i++)
    {
      if (!is_user_vaddr((const void *)stack_args))
	      sys_exit(-1);

      // Copy arg from stack to buffer
      args[i] = stack_args[i];
    }
}

/* Is the ptr valid? */
static bool is_valid_ptr(const void* ptr)
{
  return(is_user_vaddr(ptr) && (ptr >= EXECUTABLE_START));
}

/* Translate user vaddr to kernal vaddr */
static void * create_kernel_ptr(const void* ptr)
{
  // Return variable
  void * kptr;

  // Exit immediately if user is accessing protected memory
  if (!is_valid_ptr(ptr) || ((kptr = pagedir_get_page(thread_current()->pagedir, ptr)) == NULL))
    sys_exit(-1);

  return kptr;
}

/* get the process file with the given file descriptor */
static struct process_file* get_process_file(int file_descriptor)
{
  /* Acquire file system lock                               */
  lock_acquire(&file_sys_lock);

  /* Get current thread                                     */
  struct thread* cur = thread_current();
  struct list_elem* cur_elem;

  /* Iterate over file list of the current thread           */
  for(cur_elem = list_begin(&cur->file_list);
      cur_elem != list_end(&cur->file_list);
      cur_elem = list_next(cur_elem))
  {
    /* Get the process file which holds the current element */
    struct process_file *pf = list_entry(cur_elem, struct process_file, elem);
    if(pf != NULL && file_descriptor == pf->file_descriptor)
      {
        /* Return the file pointer if descriptors match     */
        lock_release(&file_sys_lock);
        return pf;  
      }
  }
  /* Return NULL if the file does not exist                 */  
  lock_release(&file_sys_lock);
  return NULL;
}

/*************************************************************
* SYSTEM CALL IMPLEMENTATIONS
*************************************************************/

/* Shutdown system */
static void sys_halt()
{
  shutdown_power_off();
}

/* Exit this process */
void sys_exit(int status)
{
  // Save exit status
  struct thread * cur = thread_current();
  cur->exit_status = status;

  // Print exit status to console
  printf("%s: exit(%d)\n", cur->name, cur->exit_status);

  // Exit the thread
  thread_exit();
}

/* Execute a command */
static pid_t sys_exec(const char* cmd_line) 
{
  // Translate ptr
  cmd_line = create_kernel_ptr(cmd_line);

  // Execute the command
  pid_t pid = process_execute(cmd_line);

  // Return PID appropriately after giving child time to load
  return thread_wait_for_load(pid) ? pid : -1;
}

/* Wait on a pid to exit*/
static int sys_wait(pid_t pid) 
{
  return process_wait(pid);
}

/* Create a new file */
static bool sys_create(const char* file, unsigned initial_size) 
{
  // Return variable
  bool success = false;

  // Translate ptr
  file = create_kernel_ptr((const void *)file);

  // Create the file
  lock_acquire(&file_sys_lock);
  success = filesys_create(file, initial_size);
  lock_release(&file_sys_lock);

  // Return success
  return success;
}

/* Remove the file from the file system */
static bool sys_remove(const char* file) 
{
  // Return variable
  bool success = false;

  // Translate ptr
  file = create_kernel_ptr((const void *)file);

  // Remove the file
  lock_acquire(&file_sys_lock);
  success = filesys_remove(file);
  lock_release(&file_sys_lock);

  // Return success
  return success;
}

/* Open the file with the given name */
static int sys_open(const char* file) 
{
  // Translate ptr
  file = create_kernel_ptr((const void *)file);

  // Local variables
  struct thread* cur = thread_current();
  struct file* f;
  struct process_file* pf;

  // Open the file
  lock_acquire(&file_sys_lock);
  f = filesys_open(file);
  if(f == NULL)
  {
    lock_release(&file_sys_lock);
    return -1;
  }

  // Create process file struct for the file
  pf = malloc(sizeof(struct process_file));
  pf->file = f;
  pf->file_descriptor = cur->fd++;
  lock_init(&pf->file_lock);
  list_push_back(&cur->file_list, &pf->elem);
  lock_release(&file_sys_lock);

  // Only return it's file descriptor
  return pf->file_descriptor;
}

/* Get the size of the specified file */
static int sys_filesize(int fd)
{
  // Local variables
  int file_size;
  struct process_file* pf;

  // Get the corresponding process file
  if((pf = get_process_file(fd)) == NULL)
    return(-1);

  // Check if file* is NULL
  lock_acquire(&(pf->file_lock));
  if(pf->file == NULL)
  {
    lock_release(&(pf->file_lock));
    return -1;
  }

  // Get file size
  file_size = file_length(pf->file);
  lock_release(&(pf->file_lock));
  return file_size;
}

/* Read from the given file */
static int sys_read(int fd, void* buffer, unsigned size)
{
  // Block reads from STDOUT
  if(fd == STDOUT_FILENO)
    return(-1);
  
  // Translate buffer
  buffer = create_kernel_ptr(buffer);

  // Read from STDIN
  if(fd == STDIN_FILENO)
  {
    uint8_t * bufIdx = (uint8_t *)buffer;
    while(bufIdx < ((uint8_t *)buffer + size))
      *bufIdx++ = input_getc();
    return size;
  }

  // Get the corresponding process file
  struct process_file* pf;
  if((pf = get_process_file(fd)) == NULL)
    return(-1);

  // Validate file*
  lock_acquire(&pf->file_lock);
  if(pf->file == NULL)
  {
    lock_release(&(pf->file_lock));
    return(-1);
  }

  // Perform actual read
  int read = -1;
  read = file_read(pf->file, buffer, size);
  lock_release(&pf->file_lock);
  return read;
}

/* Write to the given file */
static int sys_write(int fd, const void* buffer, unsigned size)
{
  // Block writes to STDIN
  if(fd == STDIN_FILENO)
    return(-1);

  // Translate buffer
  buffer = create_kernel_ptr(buffer);

  // Write entire buf to console in one go
  if(fd == STDOUT_FILENO)
  {
    putbuf((const char *)buffer, size);
    return(size);
  }

  // Get the corresponding process file
  struct process_file* pf;
  if((pf = get_process_file(fd)) == NULL)
    return(-1);

  // Validate file*
  lock_acquire(&pf->file_lock);
  if(pf->file == NULL)
  {
    lock_release(&(pf->file_lock));
    return(-1);
  }

  // Perform actual write
  int written = -1;
  written = file_write(pf->file, buffer, size);
  lock_release(&pf->file_lock);
  return written;
}

/* Set the read/write position in the given file to the specified position */
static void sys_seek(int fd, unsigned position)
{
  // Get the corresponding process file
  struct process_file* pf;
  if((pf = get_process_file(fd)) == NULL)
    return;

  // Validate file*
  lock_acquire(&(pf->file_lock));
  if(pf->file == NULL)
  {
    lock_release(&(pf->file_lock));
    return;
  }

  // Perform actual seek
  file_seek(pf->file, position);
  lock_release(&(pf->file_lock));
}

/* Get the current read/write position in the given file */
static unsigned sys_tell(int fd)
{
  // Get the corresponding process file
  struct process_file* pf;
  if((pf = get_process_file(fd)) == NULL)
    return(-1);

  // Validate file*
  lock_acquire(&pf->file_lock);
  if(pf->file == NULL)
  {
    lock_release(&(pf->file_lock));
    return -1;
  }

  // Perform actual tell
  off_t pos = file_tell(pf->file);
  lock_release(&pf->file_lock);
  return pos;
}

/* Close the specified file */
static void sys_close(int fd) 
{
  // Block closing STDOUT & STDIN
  if((fd == STDIN_FILENO) || (fd == STDOUT_FILENO))
    return;

  // Get the corresponding process file
  struct process_file* pf;
  if((pf = get_process_file(fd)) == NULL)
    return;

  // Validate file*
  lock_acquire(&pf->file_lock);
  if(pf->file == NULL)
  {
    lock_release(&pf->file_lock);
    return;
  }

  // Perform actual removal
  file_close(pf->file);
  list_remove(&pf->elem);
  lock_release(&pf->file_lock);
  free(pf);
}
