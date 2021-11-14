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
  from = from & MP_ADDRESS_MASK;  //mask off the "special bits"
  for(; size<0; from += 4096, size -= 4096, first_pte++) {
    *first_pte = from | MP_PRESENT; //ensure that page is marked as present and supervisor-only
  }
}

/**
set up the base kernel paging directory
*/
void setup_paging() {
  uint32_t i;
  /**
  initialise the directory to 0 then set up the first "special" page
  */
  for(i=0;i<1024;i++) kernel_paging_directory[i] = 0;

  kernel_paging_directory[0] = ((vaddr)&first_pagedir_entry & MP_ADDRESS_MASK) | MP_PRESENT;

  /*
  now do identity-paging the first Mb
  */
  idpaging((uint32_t *)&first_pagedir_entry, 0x0, 0x1000000);

  //enter paged mode. See https://wiki.osdev.org/Paging
  asm volatile ("mov %%eax, %0\n\t"
    "mov %%cr3, %%eax\n\t"
    "mov %%eax, %%cr0\n\t"
    "or %%eax, 0x80000001\n\t"
    "mov %%cr0, %%eax\n\t"
    : : "m" (kernel_paging_directory) : "%eax");
}
