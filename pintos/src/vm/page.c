#include "page.h"

void init(void)
{
  list_init(&sup_page_table);
  list_init(&frame_table);
  list_init(&file_mappings_table);
}

void* create_frame(struct* sup_page page)
{
  void* addr = palloc_get_page(PAL_USER);
  if (frm != NULL)
  {
    struct frame frm = malloc(sizeof(struct frame));
    frame->addr = addr;
    frame->page = page;
    list_push_back(&frame_table, frame->elem);
  }
}

void* free_frame(void* addr)
{
  bool found = FALSE;
  struct list_elem* e= list_begin(&frame_table)
  while (!found && e != list_end(&frame_table))
  {
    struct frame* frm = list_entry(e, struct frame, elem);
    if (frm->addr == addr)
    {
      list_remove(e);
      free(frm);
      palloc_free_page(addr);
      found = TRUE;
    }
  }
}
