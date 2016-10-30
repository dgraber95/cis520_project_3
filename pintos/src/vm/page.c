#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "page.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"

static struct frame * create_frame(void * addr);

static struct sup_page * create_sup_page(void * addr);
static void free_sup_page(void * addr);

void frame_init(void)
{
  list_init(&sup_page_table);
  list_init(&frame_table);
  list_init(&file_mappings_table);
}

void * frame_alloc()
{
  // Attempt to allocate frame from user pool
  void * addr = palloc_get_page(PAL_USER);

  // We obtained a free page; add to frame table
  if (addr != NULL)
  {
    // Create frame and add to frame table
    struct frame * frm = create_frame(addr);
    return frm->addr;
  }
  // No free frames: evict one

  // Panic the kernel for now
  ASSERT(addr != NULL);

  // Search the sup page table for addr, if we find it, point the frame to that entry, otherwise, create a new one
}

static struct frame * create_frame(void * addr)
{
  // Create new struct frame
  ASSERT(addr != NULL);
  struct frame * frm = malloc(sizeof(struct frame));

  // Initialize frame
  frm->addr = addr;
  frm->sup_page = create_sup_page(addr);

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
  free_sup_page(&frm->sup_page);

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

void frame_swap(struct sup_page * page)
{
  //TODO: implement swap
  
}

static struct sup_page * create_sup_page(void * addr)
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

static void free_sup_page(void * addr)
{
  // Get the sup_page table entry corresponding to this address
  ASSERT(addr != NULL);
  struct sup_page * sp = get_sup_page(addr);
  ASSERT(sp != NULL);

  // Remove from list and free resources
  list_remove(&sp->elem);
  free(sp);
}

struct sup_page * get_sup_page(void * addr)
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
