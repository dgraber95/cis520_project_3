#include "lib/kernel/list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct list sup_page_table;
struct list frame_table;
struct list file_mappings_table;

struct sup_page
{
  void * addr;
  bool dirty_bit;

  struct list_elem elem;
};

struct frame
{
  void * addr;
  struct sup_page * sup_page;

  struct list_elem elem;
};

struct file_mapping
{

  struct list_elem elem;
};


void frame_init(void);

void * frame_alloc();
void free_frame(void* addr);
struct frame * frame_get(void * addr);
void frame_swap(struct sup_page * page);

struct sup_page * get_sup_page(void * addr);
