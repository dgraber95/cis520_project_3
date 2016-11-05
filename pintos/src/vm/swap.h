#include "devices/block.h"
#include "lib/kernel/list.h"
#include "threads/vaddr.h"

#define SECTORS_PER_PAGE (PGSIZE/BLOCK_SECTOR_SIZE)
#define SWAP_SIZE 2000000

struct list swap_table;
struct block* swap_block;

struct swap_slot
{
    void * addr;
    bool free;
    struct list_elem elem;
};

int swap_push(void* src_frame);
void swap_fetch(int swap_index, void* dst_frame);
struct swap_slot* swap_get_slot(int swap_index);
