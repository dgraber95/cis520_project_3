#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "page.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/kernel/list.h"

static struct frame * frame_create(void * addr);
static struct frame * frame_get_next_eviction_candidate(void);
static void frame_load(struct frame * frm, struct sup_page * page);
static void frame_evict(struct frame * frm);

static struct sup_page * sup_page_create(void * addr);
static void sup_page_free(void * addr);
static bool sup_page_dirty(struct sup_page * page);
static void sup_page_backup(struct sup_page * page);

void frame_init(void)
{
  list_init(&sup_page_table);
  list_init(&frame_table);
  list_init(&file_mappings_table);
}

void * frame_alloc(void)
{
  // Attempt to allocate frame from user pool
  void * addr = palloc_get_page(PAL_USER);

  // We obtained a free page; add to frame table
  if (addr != NULL)
  {
    // Create frame and add to frame table
    struct frame * frm = frame_create(addr);
    return frm->addr;
  }
  // No free frames: evict one

  // Panic the kernel for now
  ASSERT(addr != NULL);

  // Search the sup page table for addr, if we find it, point the frame to that entry, otherwise, create a new one
}

static struct frame * frame_create(void * addr)
{
  // Create new struct frame
  ASSERT(addr != NULL);
  struct frame * frm = malloc(sizeof(struct frame));

  // Initialize frame
  frm->addr = addr;
  frm->sup_page = sup_page_create(addr);

  // Add to frame table
  list_push_back(&frame_table, &frm->elem);
  return frm;
}

void free_frame(void * addr)
{
  // Get the frame corresponding to the address being freed
  ASSERT(addr != NULL);
  struct frame * frm = frame_get(addr);
  ASSERT(frm != NULL);

  // Remove from frame table
  list_remove(&frm->elem);

  // Free the sup page referenced in this frame
  sup_page_free(&frm->sup_page);

  // Free actual space allocated for frame
  palloc_free_page(addr);
  free(frm);
}

struct frame * frame_get(void * addr)
{
  // Local variables
  struct list_elem * e;
  struct frame * frm;

  // Iterate through supplemental page table
  for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e))
    {
      // Get current list entry
      frm = list_entry(e, struct frame, elem);
      
      // Return the frame if it's the address requested
      if(frm->addr == addr)
        return frm;
    }

    // Unable to find the addr specified
    return NULL;
}

static struct frame * frame_get_next_eviction_candidate(void)
{
  struct frame* frm;
  struct frame* class1 = NULL;
  struct frame* class2 = NULL;
  uint32_t* pd = thread_current()->pagedir;  

  for (struct list_elem* e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e))
  {
      frm = list_entry(e, struct frame, elem);

      if( !pagedir_is_accessed(pd, frm->sup_page->addr) &&
          !pagedir_is_dirty(pd, frm->sup_page->addr) )
        return frm;
      else if ( (class1 != NULL) && 
                !pagedir_is_accessed(pd, frm->sup_page->addr) )
        class1 = frm;
      else if ( (class1 != NULL) && 
                (class2 != NULL) && 
                !pagedir_is_dirty(pd, frm->sup_page->addr) )
        class2 = frm;
  }

  if (class1)
    return class1;
  else if (class2)
    return class2;
  else 
    return frm;
}

static void frame_load(struct frame * frm, struct sup_page * page)
{
  // No data to load: zero out frame's space
  if(page->addr == NULL)
  {
    memset(frm->addr, 0, PGSIZE);
  }

  //TODO: load from swap or file
  else
  {

  }

  //TODO: Mark page as "in a frame"
}

static void frame_evict(struct frame * frm)
{
  // Local variable
  struct sup_page * evicted_page = frm->sup_page;

  //Check if current page is dirty
  if(pagedir_is_dirty(frm->addr, evicted_page->addr))
  {
    sup_page_backup(evicted_page);
  }

  // Mark old page as not present so subsequent accesses will trigger page fault
  pagedir_clear_page(thread_current()->pagedir, evicted_page->addr);
}

void frame_swap(struct sup_page * new_page)
{
  // Get eviction candidate
  struct frame * frm = frame_get_next_eviction_candidate();

  // Kick current page out of frame
  frame_evict(frm);

  // Fetch page into actual memory space
  frame_load(frm, new_page);

  // Map page into user's virtual memory
  //TODO: figure out if page is actually writable
  pagedir_set_page(thread_current()->pagedir, new_page->addr, frm->addr, true);
}

static struct sup_page * sup_page_create(void * addr)
{
  // Create new struct sup_page
  ASSERT(addr != NULL);
  struct sup_page * sp = malloc(sizeof(struct sup_page));

  // Initialize the sup_page table entry
  sp->addr = addr;
  sp->dirty_bit = false;

  // Add to supplemental page table
  list_push_back(&sup_page_table, &sp->elem);
  return sp;
}

static void sup_page_free(void * addr)
{
  // Get the sup_page table entry corresponding to this address
  ASSERT(addr != NULL);
  struct sup_page * sp = sup_page_get(addr);
  ASSERT(sp != NULL);

  // Remove from list and free resources
  list_remove(&sp->elem);
  free(sp);
}

struct sup_page * sup_page_get(void * addr)
{
  // Local variables
  struct list_elem * e;
  struct sup_page * sp;

  // Iterate through supplemental page table
  for (e = list_begin(&sup_page_table); e != list_end(&sup_page_table); e = list_next(e))
    {
      // Get current list entry
      sp = list_entry(e, struct sup_page, elem);
      
      // Return the sup page table entry if it's the address requested
      if(sp->addr == addr)
        return sp;
    }

    // Unable to find the addr specified
    return NULL;
}

static bool sup_page_dirty(struct sup_page * page)
{
  ASSERT(page != NULL);
  return pagedir_is_dirty(thread_current()->pagedir, page->addr);
}

static void sup_page_backup(struct sup_page * page)
{
  ASSERT(page != NULL);
  // TODO: write the page out to its backing store
}
