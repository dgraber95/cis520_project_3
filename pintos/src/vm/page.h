#include "lib/kernal/list.h"

struct list sup_page_table;
struct list frame_table;
struct list file_mappings_table;

struct sup_page
{

  struct list_elem elem;
};

struct frame
{
  bool free;
  void* addr;
  struct sup_page* page;

  struct list_elem elem;
};

struct file_mapping
{

  struct list_elem elem;
};


