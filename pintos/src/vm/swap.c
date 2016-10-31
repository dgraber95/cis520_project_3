#include "swap.h"
#include "devices/block.h"

void swap_init(void)
{ 
    list_init(&swap_table);
    swap_block = block_get_role(BLOCK_SWAP);
}

void swap_read(int swap_index, void* dst_frame)
{
  for (int i = 0; i < SECTORS_PER_PAGE; i++)
  {
    block_read(swap_block, used_index * SECTORS_PER_PAGE + i,
    (uint8_t *) frame + i * BLOCK_SECTOR_SIZE);
  }
}