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
