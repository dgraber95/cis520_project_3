#include "swap.h"
#include "devices/block.h"

void swap_init(void)
{ 
    list_init(&swap_table);
    swap_block = block_get_role(BLOCK_SWAP);
}

void swap_fetch(int swap_index, void* dst_frame)
{
  struct swap_slot* ss = swap_get_slot(swap_index);

  ASSERT(ss != NULL);
  ASSERT(ss->free == false);

  ss->free = true;

  for (int i = 0; i < SECTORS_PER_PAGE; i++)
  {
    block_read(swap_block, swap_index * SECTORS_PER_PAGE + i, (void*)((int)dst_frame + i * BLOCK_SECTOR_SIZE));
  }
}

int swap_push(int swap_index, void* dst_frame)
{

}

struct swap_slot* swap_get_slot(int slot_index)
{
  struct list_elem * e;
  struct swap_slot* ss;

  for(int i = 0, e = list_begin(&swap_table); e != list_end(&swap_table) && i < slot_index; i++, e = list_next(e))

  if(i == slot_index){
    return list_entry(e, struct swap_slot, elem);  
  }
  return NULL;
}

int swap_get_free()
{
  struct list_elem * e;
  struct swap_slot* ss;
  for(int i = 0, e = list_begin(&swap_table); e != list_end(&swap_table); e = list_next(e), i++)
  {
    ss = list_entry(e, struct swap_slot, elem);
    if(ss->free == true)
    {
      return ss;
    }
  }

  ASSERT(list_size(&swap_table) < SWAP_SIZE/PGSIZE);
  ss = malloc(sizeof(struct swap_slot));
  list_push_back()
}