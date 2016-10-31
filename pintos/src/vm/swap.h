#include "lib/kernel/list.h"



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
