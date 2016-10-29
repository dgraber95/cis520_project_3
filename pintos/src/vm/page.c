#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "page.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"

void frame_init(void)
{
  list_init(&sup_page_table);
  list_init(&frame_table);
  list_init(&file_mappings_table);
}

void* create_frame()
{
  // Attempt to allocate frame from user pool
  void* addr = palloc_get_page(PAL_USER);

  // We obtained a free page; add to frame table
  if (addr != NULL)
  {
    struct frame* frm = malloc(sizeof(struct frame));
    frm->addr = addr;
    //frm->page = page;
    list_push_back(&frame_table, &frm->elem);
  }
  // No free frames: evict one
  else
  {
    // Panic the kernel for now
    ASSERT(addr != NULL);
  }

  return addr;
}

void free_frame(void* addr)
{
  // Get first element in frame table
  struct list_elem* e = list_begin(&frame_table);

  // Iterate through frame table
  while (e != list_end(&frame_table))
  {
    // Obtain frame* from current iteration
    struct frame* frm = list_entry(e, struct frame, elem);

    // Found the corresponding frame: free it
    if (frm->addr == addr)
    {
      list_remove(e);
      palloc_free_page(addr);
      free(frm);
      return;
    }

    // Move to next element
    e = list_next(e);
  }
}
