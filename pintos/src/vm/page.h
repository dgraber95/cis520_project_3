#include "lib/kernel/list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct list sup_page_table;
struct list frame_table;
struct list file_mappings_table;

struct sup_page
{
  bool dirty_bit;
  void* virtual_page;

  struct list_elem elem;
};

struct frame
{
  void* addr;
  struct sup_page* page;

  struct list_elem elem;
};

struct file_mapping
{

  struct list_elem elem;
};


void init(void);
void* create_frame(uint8_t page);
void free_frame(void* addr);
