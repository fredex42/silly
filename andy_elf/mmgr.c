#include <types.h>
#include "mmgr.h"

static uint32_t __attribute__ ((aligned(4096))) kernel_paging_directory[1024];  //root paging directory
static uint32_t __attribute__ ((aligned(4096))) first_pagedir_entry[1024];    //first entry in the page table, this covers the first 4Mb

void idpaging(uint32_t *first_pte, vaddr from, int size);

/**
Internal function to set up identity paging for a given memory area.
This assumes that page sizes are 4k.
@param first_ptr - pointer to the first page directory entry to set.
@param from      - start of the physical ram area to identity page
@param size      - how many bytes to configure
*/
void idpaging(uint32_t *first_pte, vaddr from, int size) {
  register uint32_t i;
  uint32_t page_count = size / 4096;
  uint32_t block_ptr = from & MP_ADDRESS_MASK;
  char range_ctr=0;
  char write_protect=0;
  //see https://wiki.osdev.org/Memory_Map_(x86). This takes the format of a collection
  //of pairs of page indices, start, end, start, end etc.  Everything
  //between a 'start' and and 'end' is write-protected. These are the 'special' ranges
  //of the first MiB.  We are allowed to write to the video framebuffer between A0000 and C0000 though,
  static uint32_t protected_pages[] = {0x7, 0xa, 0x80, 0xA0, 0xC0, 0xFF};
  #define PROTECTED_PAGES_COUNT 5
  for(i=0;i<page_count;i++) {
    if(range_ctr<PROTECTED_PAGES_COUNT && i==protected_pages[range_ctr]) {
      ++range_ctr;
      write_protect = !write_protect;
    }
    //if(write_protect) kprintf("DEBUG: protecting page %d.  ", i);
    register uint32_t value = write_protect ? block_ptr | MP_PRESENT : block_ptr | MP_PRESENT| MP_READWRITE ;
    //kprintf("DEBUG: page %d value 0x%x   \r\n", i, value);
    first_pte[i] = value;
    block_ptr+=4096;
  }
}

/**
set up the base kernel paging directory
*/
void setup_paging() {
  register uint32_t i;
  /**
  initialise the directory to 0 then set up the first "special" page
  */
  for(i=0;i<1024;i++) kernel_paging_directory[i] = 0;
  for(i=0;i<1024;i++) first_pagedir_entry[i] = 0;

  kernel_paging_directory[0] = ((vaddr)&first_pagedir_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;

  /*
  now do identity-paging the first Mb
  */
  idpaging((uint32_t *)&first_pagedir_entry, 0x0, 0x100000);


  kprintf("Page 0x00 value is %x\r\n", first_pagedir_entry[0x0]);
  kprintf("Page 0x7F value is %x\r\n", first_pagedir_entry[0x7F]);
  kprintf("Page 0x80 value is %x\r\n", first_pagedir_entry[0x80]);
  kprintf("Page 0xBE value is %x\r\n", first_pagedir_entry[0xBE]);
  /*
  enter paged mode. See https://wiki.osdev.org/Paging
  NOTE! operands are the other way around to nasm! (i.e. mov src, dest)
*/
  asm volatile ("lea %0, %%eax\n\t"
    "mov %%eax, %%cr3\n\t"
    "mov %%cr0, %%eax\n\t"
    "or $0x80010001, %%eax\n\t" //enable paging and write-protect in ring 0. See sys/x86_control_registers.h for bitfield details.
    "mov %%eax, %%cr0\n\t"
    : : "m" (kernel_paging_directory) : "%eax");

}
