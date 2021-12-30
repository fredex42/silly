#include <sys/mmgr.h>
#include <types.h>
#include <stdio.h>
#include "cfuncs.h"

void simple_alloc_fill_dealloc(size_t page_count) {
    register size_t i,j;
    size_t x=0,y=0;

    char *ptr = (char *)vm_alloc_pages(NULL, page_count, MP_READWRITE);
    kprintf("Returned ptr is 0x%x\r\n",ptr);
    kputs("Attempting to write those pages with a number sequence...\r\n");

    j=1;
    for(i=0;i<page_count*PAGE_SIZE;i++) {
      ptr[i] = (char)j;
      j++;
      if(j>9) j=1;
      x++;
      if(x>=PAGE_SIZE) {
        kputs("P");
        x=0;
        y++;
      }
      if(y>50) {
        kputs("\r\n");
        y=0;
      }
    }

    kputs("Reading back...\r\n");
    for(i=0; i<page_count*PAGE_SIZE;i++) {
      char ch = ptr[i];
    }

    kputs("Deallocating...\r\n");
    vm_deallocate_physical_pages(NULL, ptr, page_count);

}

void run_inkernel_memory_tests()
{
  kputs("Inkernel memory tests...\r\n");
  kputs("Test 1. Attempting to allocate a block of 2 pages for readwrite....\r\n");
  simple_alloc_fill_dealloc(2);
  kputs("Done.");

  kputs("Test 2. Attempting to allocate a block of 512 pages for readwrite...\r\n");
  simple_alloc_fill_dealloc(512);
  kputs("Done.");
}
