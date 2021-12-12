#include <types.h>
#include <sys/mmgr.h>
#include <stdio.h>
#include "cfuncs.h"
#include "panic.h"

static uint32_t __attribute__ ((aligned(4096))) kernel_paging_directory[1024];  //root paging directory
static uint32_t __attribute__ ((aligned(4096))) first_pagedir_entry[1024];      //first entry in the page table, this covers the first 4Mb

static uint32_t __attribute__ ((aligned(4096))) physical_page_directory[1024];  //"inverse" memory map describing which pages are in use

/** gets the page directory index for the given address (either virtual or physical) */
#define ADDR_TO_PAGEDIR_IDX(addr) (size_t)vaddr / 0x400000;
/** gets the page index within the page directory for the given address */
#define ADDR_TO_PAGEDIR_OFFSET(addr) (vaddr - ((size_t)vaddr % 0x400000) ) / 0x1000

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

/**
Searches the physical memory map to find the requested number of free pages.
Found pages are marked as "allocated" before returning.
Returns (physical) memory pointers to each in the array pointed to by the "blocks" argument.
This must be large enough to accomodate `page_count` pointers.
Return value is the number of pages found. If this is < page_count it indicates an
out-of-memory condition.
*/
uint32_t find_free_physical_pages(uint32_t page_count, void **blocks)
{
  register int i,j;
  uint32_t current_entry;
  uint32_t * current_page;

  uint32_t found_pages = 0;

  for(i=0;i<1024;i++) {
    current_entry = physical_page_directory[i];
    current_page = (uint32_t *)(current_entry & MP_ADDRESS_MASK);

    if(!current_page & MP_PRESENT) continue;  //if the page is not present then don't enumerate

    for(j=0;j<1024;j++) {
      if(physical_page_directory[i] & MP_DIRTY) continue; //if there are no free pages in this directory entry then go straight to the next one
      if(! ((uint32_t)current_page & MP_DIRTY) ) {
        blocks[found_pages] = (void *)(i*0x400000 + j*4096);
        ++found_pages;
        if(found_pages==page_count) return found_pages;
      }
    }
  }
  return found_pages; //we ran out of memory. Return the number of pages that we managed to get.
}

/**
Marks the provided list of pages as "free".
First argument is the number of pages in the list to free.
Second argument is a pointer to a list of blocks to free. This must have the same
number of entries as page_count.
*/
uint32_t unmark_physical_pages(uint32_t page_count, void **blocks)
{
  register uint32_t i;
  vaddr phys_addr;

  for(i=0;i<page_count;i++) {
    phys_addr = (vaddr)blocks[i];

  }
}
/**
map a page of physical memory into the kernel's memory space.
this is a basic low-level function and no checks are performed, careful!
Arguments:
- phys_addr - physical memory address to map from. Must be on a 4k boundary or it will be truncated.
- pagedir_idx - index in the page directory to map it to (the 4Mb block)
- pageent_idx - index in the relevant page index to map it to (the 4Kb block)
- flags - MP_ flags to indicate requested protection
Returns:
- the virtual memory location of the first byte of the page that the physical memory was mapped to
*/
void * k_map_page(void * phys_addr, uint16_t pagedir_idx, uint16_t pageent_idx, uint32_t flags)
{
  if(pagedir_idx!=0) return NULL; //not currently supported!
  if(pageent_idx>1023) return NULL; //out of bounds
  first_pagedir_entry[pageent_idx] = ((vaddr)phys_addr & MP_ADDRESS_MASK) | MP_PRESENT | flags;

  return (void *)(pageent_idx*4096);
}

/**
finds the index of the next page not marked "present"
*/
int16_t k_find_next_unallocated_page(uint16_t start_from)
{
  for(register uint16_t i=start_from; i<1024; i++) {
    if((first_pagedir_entry[i] & MP_PRESENT)==0) return i;
  }
  return -1;
}

/**
unmap a page of physical memory by removing it from the page table and setting the
page to "not present"
*/
void k_unmap_page(uint16_t pagedir_idx, uint16_t pageent_idx)
{
  if(pagedir_idx!=0) return;  //not currently supported!
  if(pageent_idx>1023) return; //out of bounds

  first_pagedir_entry[pageent_idx] = 0;
}

/**
if the given physical address is already mapped, then returns the virtual address
that it is mapped to.  If not then it finds an unallocated page, maps it and
returns that.
*/
void* k_map_if_required(void *phys_addr, uint32_t flags)
{
  //save the offset within the page
  uint32_t page_offset = (uint32_t) phys_addr & ~MP_ADDRESS_MASK;
  //now search the page directory to try and find the physical address
  uint32_t page_value = (uint32_t) phys_addr & MP_ADDRESS_MASK;

  for(register int i=0;i<1023;i++) {
    if(first_pagedir_entry[i] & MP_ADDRESS_MASK == page_value && first_pagedir_entry[i] & MP_PRESENT) {
      vaddr v_addr = (vaddr)i*4096 + page_offset;
      return (void *)v_addr;
    }
  }

  int16_t page_to_allocate = k_find_next_unallocated_page(0);
  void *mapped_page_addr = k_map_page((void *)page_value, 0, page_to_allocate, flags);
  return mapped_page_addr +page_offset;
}

/**
parses the memory map obtained from BIOS INT 0x15, EAX = 0xE820
by the bootloader.
Params:
  - ptr - pointer to where the bootloader left the map. This is expected
  to start with a 1-byte word containing the number of records and then a number
  of 24-byte records conforming to struct MemoryMapEntry
*/
void parse_memory_map(struct BiosMemoryMap *ptr)
{
  uint8_t entry_count = ptr->entries;

  kprintf("Detected memory map has %d entries:\r\n", entry_count);
  for(register int i=0;i<entry_count;i++){
    struct MemoryMapEntry *e = (struct MemoryMapEntry *)&ptr[2+i*24];
    uint32_t page_count = e->length / 4096;
    kprintf("%x | %x | %d: ", (size_t)e->base_addr, (size_t)e->length, page_count);
    switch (e->type) {
      case MMAP_TYPE_USABLE:
        kputs("Free Memory (1)\r\n");
        break;
      case MMAP_TYPE_RESERVED:
        kputs("Reserved (2)\r\n");
        break;
      case MMAP_TYPE_ACPI_RECLAIMABLE:
        kputs("Reclaimable (3)\r\n");
        break;
      case MMAP_TYPE_ACPI_NONVOLATILE:
        kputs("Nonvolatile (4)\r\n");
        break;
      case MMAP_TYPE_BAD:
        kputs("Faulty (5)\r\n");
        break;
      default:
        kputs("Unknown\r\n");
        break;
    }
  }
}

struct BiosMemoryMap * allocate_physical_map(struct BiosMemoryMap *ptr)
{
  register int i;
  //find the highest value in the physical memory map. We must allocate up to here.
  uint8_t entry_count = ptr->entries;

  size_t highest_value = 0;

  for(register int i=0;i<entry_count;i++){
    struct MemoryMapEntry *e = (struct MemoryMapEntry *)&ptr[2+i*24];
    size_t entry_limit = (size_t) e->base_addr + (size_t)e->length;
    if(entry_limit>highest_value) highest_value = entry_limit;
  }

  kprintf("Detected %d Mb of physical RAM\r\n", highest_value/1048576);

  //each page directory covers 1024 4kb pages, i.e. 4Mb.
  size_t directories_to_allocate = highest_value / 4194304;
  kprintf("Allocating %d pages to physical ram map\r\n", directories_to_allocate);
  /**
  initialise the physical memory map.
  */
  for(i=0;i<1024;i++) physical_page_directory[i] = 0;

  //since we don't have a physical map yet, we are going to have to rely on some
  //intuition and prerequisites here.....
  //1. We assume that the whole first Mb is identity-mapped
  //2. We assume that the kernel itself (code and static data) fits within 16 pages (64k)
  //3. We assume a standard memory layout, i.e. the kernel is loaded from 0x7e00 (page 0x07)
  //and that there is a memory hole at 0x80000 (page 0x80=128)
  //Therefore, we can start out pseudo-allocation from page 24 (=0x18) but can't go beyond 0x80 - limit of 0x5c = 92 pages
  if(directories_to_allocate>92) k_panic("Exceeded physical ram directory allocation\r\n"); //this won't return

  size_t current_alloc_ptr = 0x18000;  //start of page 0x18. We assume that this is identity-mamped
  for(register int i=0;i<directories_to_allocate;i++){
    physical_page_directory[i] = (current_alloc_ptr & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE | MP_GLOBAL;
    current_alloc_ptr += 0x1000;
  }

  /**
  initial populate of the physical memory map. We use the "dirty" flag in the physical memory
  map to indicate "in use".
  Page  0         (0x0     -> 0x1000) contains our code and data.
  Pages 0x7->0xa  (0x7000  -> 0xa000) are reserved
  Pages 0x80->0xff(0x80000 -> 0x100000) are reserved
  */
  uint32_t *first_physdir_entry = (uint32_t *)(physical_page_directory[0] & MP_ADDRESS_MASK);

  for(i=7;i<0x0a;i++) first_physdir_entry[i] = MP_PRESENT | MP_READWRITE | MP_DIRTY;
  for(i=0x80;i<0xFF;i++) first_physdir_entry[i] = MP_PRESENT | MP_READWRITE | MP_DIRTY;
  for(i=0x18;i<directories_to_allocate+0x18;i++) first_physdir_entry[i] = MP_PRESENT | MP_READWRITE | MP_DIRTY;
}

/**
sets the "readonly" and "dirty" flag on the protected memory regions
*/
void apply_memory_map_protections(struct BiosMemoryMap *ptr)
{
  uint8_t entry_count = ptr->entries;
  kputs("Applying memory protections\r\n");

  for(register int i=0;i<entry_count;i++){
    struct MemoryMapEntry *e = (struct MemoryMapEntry *)&ptr[2+i*24];
    uint32_t page_count = e->length / 4096;
    switch(e->type) {
      case MMAP_TYPE_RESERVED:
      case MMAP_TYPE_ACPI_RECLAIMABLE:
      case MMAP_TYPE_ACPI_NONVOLATILE:
      case MMAP_TYPE_BAD:
        _reserve_memory_block(e->base_addr, page_count);
        break;
      default:
        break;
    }
  }
}

/**
internal function called from apply_memory_map_protections to protect a given
memory block
*/
void _reserve_memory_block(void *base_addr, uint32_t page_count) {
  size_t base_phys_page = ADDR_TO_PAGEDIR_IDX(base_addr);

  for(register size_t i=0;i<page_count;i++) {

  }
}
