#include <types.h>
#include <sys/mmgr.h>

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
  void *mapped_page_addr = k_map_page(page_value, 0, page_to_allocate, flags);
  return &mapped_page_addr[page_offset];
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
    kprintf("%x | %x | %d: ", (uint32_t)e->base_addr, (uint32_t)e->length, page_count);
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
