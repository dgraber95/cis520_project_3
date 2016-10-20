#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "page.h"
#include "threads/palloc.h"
#include "threads/malloc.h"

void init(void);
void* create_frame(struct sup_page* page);
void* free_frame(void* addr);

void init(void)
{
  list_init(&sup_page_table);
  list_init(&frame_table);
  list_init(&file_mappings_table);
}

void* create_frame(struct sup_page* page)
{
  void* addr = palloc_get_page(PAL_USER);
  if (addr != NULL)
  {
    struct frame* frm = malloc(sizeof(struct frame));
    frm->addr = addr;
    frm->page = page;
    list_push_back(&frame_table, frm->elem);
  }

return addr;
}

void* free_frame(void* addr)
{
  bool found = 0;
  struct list_elem* e= list_begin(&frame_table);

  while (!found && e != list_end(&frame_table))
  {
    struct frame* frm = list_entry(e, struct frame, elem);
    if (frm->addr == addr)
    {
      list_remove(e);
      free(frm);
      palloc_free_page(addr);
      found = 1;
    }
    e = list_next(e);
  }

return addr;
}
