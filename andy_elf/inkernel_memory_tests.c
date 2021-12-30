#include <sys/mmgr.h>
#include <types.h>
#include <stdio.h>
#include "cfuncs.h"

void run_inkernel_memory_tests()
{
  register size_t i,j;

  kputs("Inkernel memory tests...\r\n");
  kputs("Attempting to allocate a block of 2 pages for readwrite....\r\n");

  char *ptr = (char *)vm_alloc_pages(NULL, 2, MP_READWRITE);
  kprintf("Returned ptr is 0x%x\r\n",ptr);
  kputs("Attempting to write those pages with a number sequence...\r\n");

  j=1;
  for(i=0;i<8192;i++) {
    ptr[i] = (char)j;
    j++;
    if(j>9) j=1;
  }

  kputs("Reading back...\r\n");
  for(i=0; i<8192;i++) {
    char ch = ptr[i];
  }

  kputs("Deallocating...\r\n");
  vm_deallocate_physical_pages(NULL, ptr, 2);

  kputs("Done.");

}
