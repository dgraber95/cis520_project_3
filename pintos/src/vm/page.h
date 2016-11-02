#include "lib/kernel/list.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int mapid_t;

struct list sup_page_table;
struct list frame_table;
struct list file_mappings_table;

int next_map_id;

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
  int fd;
  mapid_t map_id;
  struct list_elem elem;
};

void frame_init(void);

void * frame_alloc(void);
struct frame * frame_create(void * addr);
void free_frame(void* addr);
struct frame * frame_get(void * addr);
void frame_swap(struct sup_page * page);

mapid_t map_file(int fd);
void unmap_file(mapid_t map_id);

struct sup_page * sup_page_get(void * addr);
