#include <types.h>
#include <memops.h>
#include <sys/mmgr.h>
#include "vfat.h"

/*
Allocates a new FATBuffer structure, with page_count 4k pages of memory available.
The data type will be FAT_TYPE_INVALID.
*/
FATBuffer *new_fatbuffer(uint16_t page_count)
{
  FATBuffer *b = (FATBuffer *) vm_alloc_pages(NULL, page_count, MP_READWRITE);
  if(b==NULL) return NULL;

  //set whole thing to 0
  memset((void *)b, 0, page_count*PAGE_SIZE);
  //point the "buffer" area to the end of the struct
  b->buffer = (void *)((vaddr)b + sizeof(FATBuffer));
  b->initial_length_bytes = page_count * PAGE_SIZE;
}
