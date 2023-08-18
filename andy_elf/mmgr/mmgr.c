#ifdef __BUILDING_TESTS
  #include <sys/types.h>
  #include <stdint.h>
  #include "include/sys/mmgr.h"
  #if __WORDSIZE == 64
    typedef uint64_t vaddr;
  #else
    typedef uint64_t vaddr;
  #endif
#else
  #include <types.h>
  #include <sys/mmgr.h>
#endif
#include <sys/pagefault.h>
#include <memops.h>
#include <stdio.h>
#include <cfuncs.h>
#include <cpuid.h>
#include "panic.h"

#include "heap.h"
#include "process.h"

#define ROOT_PAGE_DIR_LOCATION        0x3000
#define FIRST_PAGEDIR_ENTRY_LOCATION  0x4000

static uint32_t *kernel_paging_directory;  //root paging directory on page 0
static uint32_t *first_pagedir_entry;    //first directory entry on page 1

//all of the page tables in the root directory get mapped into this area.
//We initialise it by configuring a page directory to cover the whole area and set up the first entry to the first_pagedir_entry value
//After that, further pages are added "just-in-time" by handling page faults to the region.
static uint32_t *flat_pagetables_ptr  = 0xF0000000;     

//this is a pointer to an array of physical_page_count PhysMapEntry objects
struct PhysMapEntry *physical_memory_map = NULL;
static size_t physical_page_count = 0;

#define __invalidate_vptr(vptr_to_invalidate) asm inline volatile("invlpg (%0)" : : "r" (vptr_to_invalidate) : "memory")
void idpaging(uint32_t *first_pte, vaddr from, int size);
void allocate_physical_map(struct BiosMemoryMap *ptr);
void parse_memory_map(struct BiosMemoryMap *ptr);
void apply_memory_map_protections(struct BiosMemoryMap *ptr);
void* vm_add_dir(uint32_t *root_page_dir, uint16_t idx, uint32_t flags);
uint32_t allocate_free_physical_pages(uint32_t page_count, void **blocks);
void *k_map_page_bytes(uint32_t *root_page_dir, void *phys_addr, void *target_virt_addr, uint32_t flags);
vaddr _mmgr_get_pd();

/**
Kickoff entrypoint
*/
void initialise_mmgr(struct BiosMemoryMap *ptr)
{
  uint32_t flags = cpuid_edx_features();
  if(flags & CPUID_FEAT_EDX_PAE) {
    kputs("CPU supports physical address extension.\r\n");
  }
  parse_memory_map(ptr);
  kputs("Setup paging....\r\n");
  setup_paging();
  kputs("Allocating physical map space...\r\n");
  allocate_physical_map(ptr);
  kputs("Applying memory map protections...\r\n");
  apply_memory_map_protections(ptr);
  kputs("Initialising pagetables area");
  initialise_flat_pagetables();
  kputs("Memory manager initialised.\r\n");
  initialise_process_table(kernel_paging_directory);
  initialise_heap(get_process(0), MIN_ZONE_SIZE_PAGES*4);
}

/**
Internal function to set up identity paging for a given memory area.
This assumes that page sizes are 4k.
@param first_ptr - pointer to the first page directory entry to set.
@param from      - start of the physical ram area to identity page
@param size      - how many bytes to configure
*/
void idpaging(uint32_t *first_pte, vaddr from, int size) {
  register uint32_t i;
  uint32_t page_count = size / PAGE_SIZE;
  uint32_t block_ptr = from & MP_ADDRESS_MASK;
  uint8_t range_ctr=0;
  uint8_t write_protect=0;
  //see https://wiki.osdev.org/Memory_Map_(x86). This takes the format of a collection
  //of pairs of page indices, start, end, start, end etc.  Everything
  //between a 'start' and and 'end' is write-protected. These are the 'special' ranges
  //of the first MiB.  We are allowed to write to the video framebuffer between A0000 and C0000 though,

  uint32_t protected_pages[] = {0x7, 0x11, 0x80, 0xA0, 0xC0, 0xFF};
  #define PROTECTED_PAGES_COUNT 5
  for(i=0;i<page_count;i++) {
    if(range_ctr<PROTECTED_PAGES_COUNT && i==protected_pages[range_ctr]) {
      ++range_ctr;
      write_protect = !write_protect;
    }
    register uint32_t value = write_protect ? block_ptr | MP_PRESENT | MP_GLOBAL : block_ptr | MP_PRESENT| MP_READWRITE | MP_GLOBAL;
    //page 6 is 1 below the kernel stack. Mark this as "not writable" so we can't clobber kernel memory
    //unfortunately this does cause a triple-fault if the stack overflows, because it's impossible to invoke an interrupt
    //with an overflowing stack.
    if(i==0x6F) value = block_ptr | MP_PRESENT ; 
    first_pte[i] = value;
    block_ptr+=PAGE_SIZE;
  }
}

/**
set up the base kernel paging directory
*/
void setup_paging() {
  register uint32_t i;
  kputs("  Initialise kernel vmem...\r\n");
  kernel_paging_directory = (uint32_t *) ROOT_PAGE_DIR_LOCATION;
  first_pagedir_entry = (uint32_t *) FIRST_PAGEDIR_ENTRY_LOCATION;

  /**
  initialise the directory to 0 then set up the first "special" page
  */
  for(i=0;i<1024;i++) kernel_paging_directory[i] = 0;
  for(i=0;i<1024;i++) first_pagedir_entry[i] = 0;

  kernel_paging_directory[0] = ((vaddr)first_pagedir_entry & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;

  kputs("  Setup ID paging...\r\n");
  /*
  now do identity-paging the first Mb
  */
  idpaging((uint32_t *)first_pagedir_entry, 0x0, 0x100000);

  kputs("  Enter paged mode...\r\n");
  mb();

  /*
  enter paged mode. See https://wiki.osdev.org/Paging
  NOTE! operands are the other way around to nasm! (i.e. mov src, dest)
*/
  asm volatile ("mov %0, %%eax\n\t"
    "mov %%eax, %%cr3\n\t"
    "mov %%cr0, %%eax\n\t"
    "or $0x80010001, %%eax\n\t" //enable paging and write-protect in ring 0. See sys/x86_control_registers.h for bitfield details.
    "mov %%eax, %%cr0\n\t"
    : : "m" (kernel_paging_directory) : "%eax");
}

/**
 * Set up the flat_pagetables region.  Only the first page is actually mapped, further ones are mapped on-demand by catching pagefaults
 * and allocating the memory as it is needed.
*/
void initialise_flat_pagetables() {
  void* phys_ptrs[1];

  vaddr *temp_ptr = 0x300000;  //we are not using the 3Mb range right now, use that as a temporary area
  vaddr temp_pagedir = (vaddr)temp_ptr >> 12;
  vaddr target_pagedir = (vaddr)flat_pagetables_ptr >> 22;

  kprintf("\r\nDEBUG initialise_flat_pagetables temp_pagedir is 0x%x target_pagedir is 0x%x\r\n", temp_pagedir, target_pagedir);

  uint32_t allocd = allocate_free_physical_pages(1, &phys_ptrs);
  if(allocd!=1 ) {
    k_panic("ERROR Unable to initialise 1 pages for flat pagetables");
  }

  first_pagedir_entry[temp_pagedir] = (vaddr)phys_ptrs[0] | MP_READWRITE | MP_PRESENT;
  mb();

  memset((void *)temp_ptr, 0, PAGE_SIZE);
  temp_ptr[0] = (vaddr)first_pagedir_entry | MP_READWRITE | MP_PRESENT;
  temp_ptr[0x3C0] = (vaddr)phys_ptrs[0] | MP_READWRITE | MP_PRESENT;
  kernel_paging_directory[target_pagedir] = (vaddr)phys_ptrs[0] | MP_READWRITE | MP_PRESENT;
  first_pagedir_entry[temp_pagedir] = 0;
  __invalidate_vptr(temp_ptr);
  mb();
}

/**
 * Returns a properly typed pointer to the page table for the given virtual address.
 * This may not yet have memory mapped to it, but that should get transparently handled by the page-fault handler
*/
uint32_t *get_pagetable(vaddr virt_address) {
  vaddr page = virt_address >> 22;
  vaddr dest = (vaddr)flat_pagetables_ptr + page * PAGE_SIZE;
  return (uint32_t *)dest;
}

/**
Searches the physical memory map to find the requested number of free pages.
Found pages are marked as "allocated" before returning.
Returns (physical) memory pointers to each in the array pointed to by the "blocks" argument.
This must be large enough to accomodate `page_count` pointers.
Return value is the number of pages found. If this is < page_count it indicates an
out-of-memory condition.
*/
uint32_t allocate_free_physical_pages(uint32_t page_count, void **blocks)
{
  uint32_t current_entry;
  uint32_t * current_page;
  uint8_t temp = 0;
  uint32_t found_pages = 0;

  for(register size_t i=0x30;i<physical_page_count;i++) {
    if(physical_memory_map[i].in_use==0) {
      if(temp<5) {
        temp++;
      }
      physical_memory_map[i].in_use=1;
      blocks[found_pages] = (void *)(i*PAGE_SIZE);
      found_pages+=1;
      if(found_pages==page_count) return page_count;  //we found enough pages
    }
  }
  return found_pages; //we ran out of memory. Return the number of pages that we managed to get.
}

/**
Marks the provided list of pages as "free".
First argument is the number of pages in the list to free.
Second argument is a pointer to a list of blocks to free. This must have the same
number of entries as page_count.
Third argument is 1 if we should set the page to zero before freeing.
*/
uint32_t deallocate_physical_pages(uint32_t page_count, void **blocks)
{
  vaddr phys_addr;

  for(register size_t i=0;i<page_count;i++) {
    phys_addr = (vaddr)blocks[i];
    size_t page_idx = phys_addr / PAGE_SIZE;
    physical_memory_map[i].in_use=0;
  }
}

/**
Convenience method that maps a physical page onto a target virtual address by calculating
the paging directory offsets
*/
void *k_map_page_bytes(uint32_t *root_page_dir, void *phys_addr, void *target_virt_addr, uint32_t flags)
{
  uint16_t pagedir_idx, pageent_idx;
  _resolve_vptr(target_virt_addr, &pagedir_idx, &pageent_idx);

  return k_map_page(root_page_dir, phys_addr, pagedir_idx, pageent_idx, flags);
}

/**
map a page of physical memory into the given memory space
this is a basic low-level function and no checks are performed, careful!
Arguments:
- root_page_dir - pointer to the root directory to map into
- phys_addr - physical memory address to map from. Must be on a 4k boundary or it will be truncated.
- pagedir_idx - index in the page directory to map it to (the 4Mb block)
- pageent_idx - index in the relevant page index to map it to (the 4Kb block)
- flags - MP_ flags to indicate requested protection
Returns:
- the virtual memory location of the first byte of the page that the physical memory was mapped to
*/
void * k_map_page(uint32_t *root_page_dir, void * phys_addr, uint16_t pagedir_idx, uint16_t pageent_idx, uint32_t flags)
{
  uint32_t *actual_root;
  uint32_t *pagedir;

  if((vaddr)root_page_dir != _mmgr_get_pd()) {
    k_panic("ERROR Not implemented yet - can't map pages in non-active directory.\r\n");
  }
  if(pagedir_idx>1023 || pageent_idx>1023) return NULL; //out of bounds

  vaddr page_ptr = (vaddr)flat_pagetables_ptr + ((vaddr)pagedir_idx << 12) + ((vaddr)pageent_idx * sizeof(uint32_t));
  kprintf("DEBUG k_map_page page ptr for %d - %d is 0x%x\r\n", pagedir_idx, pageent_idx, page_ptr);
  *(uint32_t *)page_ptr = (vaddr)phys_addr | MP_PRESENT | flags;
  vaddr vptr = (vaddr)pagedir_idx << 22 | (vaddr)pageent_idx << 12;

  kprintf("DEBUG k_map_page successfully mapped physical pointer 0x%x to vptr 0x%x\r\n", phys_addr, vptr);
  return (void *)vptr;
}

/**
adds a new blank directory to the virtual memory table
Arguments:
- root_page_dir - pointer to the root of the virtual memory directory
- idx           - number of the directory to allocate into
- flags         - any additional MP_ flags to apply to the memory. MP_PRESENT is always applied.
Returns:
- Pointer to the allocated memory, mapped into kernel-space
*/
// void* vm_add_dir(uint32_t *root_page_dir, uint16_t idx, uint32_t flags)
// {
//   void *phys_ptr=NULL; //only one entry
//   kprintf("DEBUG Adding a new directory into the virtual memory table at 0x%d\r\n", idx);
//   size_t allocd = allocate_free_physical_pages(1, &phys_ptr);
//   kprintf("DEBUG allocd %d pages at 0x%x\r\n", allocd, phys_ptr);

//   if(allocd!=1) k_panic("Could not allocate physical ram for directory");
  
//   void *allocd_page_content = k_map_if_required(NULL, phys_ptr, MP_READWRITE);  //even if we are allocating for another process, it's the kernel that needs visibility of this.
//   kprintf("DEBUG mapped RAM allocated from 0x%x to 0x%x\r\n", phys_ptr, allocd_page_content);

//   memset(allocd_page_content, 0, PAGE_SIZE);  //ensure that the page content is blanked

//   root_page_dir[idx] = ((size_t) phys_ptr & MP_ADDRESS_MASK) | MP_PRESENT | flags;
//   kprintf("DEBUG setting page dir index %d to 0x%x\r\n", idx, root_page_dir[idx]);
//   uint32_t *pagedir_entries = (uint32_t *)PAGEDIR_ENTRIES_LOCATION;
//   pagedir_entries[idx] = allocd_page_content;

//   return allocd_page_content;
// }

void *k_map_next_unallocated_pages(uint32_t flags, void **phys_addr, size_t pages)
{
  return vm_map_next_unallocated_pages(kernel_paging_directory, flags, phys_addr, pages);
}

/**
Maps the given physical address(es) into the next (contigous block of) free page of the given root page directory.
You should ensure that interrupts are disabled when calling this function.
Arguments:
- root_page_dir - pointer to the root page directory to use.  NULL means use the kernel paging directory.
- flags - MP_* flags to apply to the allocated Memory
- phys_addr - a pointer to an array of physical RAM pointers. These must be 4k aligned, and available.
- pages - the length of the phys_addr array.
Returns:
- a virtual memory pointer to the mapped region, or NULL if there were not enough pages of
virtual memory to complete or `pages` was 0.
*/
void * vm_map_next_unallocated_pages(uint32_t *root_page_dir, uint32_t flags, void **phys_addr, size_t pages)
{
  register int16_t i,j;
  register size_t p = 0;
  //uint32_t *pagedir_entries = (uint32_t *)PAGEDIR_ENTRIES_LOCATION;
  size_t continuous_page_count = 0;
  uint16_t starting_page_dir = 0xFFFF;
  uint16_t starting_page_off = 0xFFFF;
  uint32_t *pagedir_entry_phys, *pagedir_entry_vptr = NULL;

  if(pages==0) return NULL; //don't try and map no Memory

  if(root_page_dir==NULL) root_page_dir = kernel_paging_directory;

  //we need to find `pages` contigous free pages of virtual memory space then map the potentially
  //discontinues phys_addr pointers onto them with the given flags.
  //then, return the vmem ptr to the first one.
  //Don't map anything into the first Mb to avoid confusion.
  for(i=0x100;i<(1024*1024);i++) {
    //i counts the page where we are
    uint32_t *pageptr = (uint32_t *)((vaddr)flat_pagetables_ptr + (vaddr)(i * sizeof(vaddr)));
    //kprintf("DEBUG vm_map_next_unallocated_pages pageptr for page %d is 0x%x\r\n", i, pageptr);
    if(! ((*pageptr) & MP_PRESENT) ) {
      if(starting_page_dir==0xFFFF) starting_page_dir = i >> 12;
      if(starting_page_off==0xFFFF) starting_page_off = (starting_page_dir << 12) | (i & 0x3FF);
      continuous_page_count++;
      //kprintf("DEBUG vm_map_next_unallocated_pages found page %d of %d at %d-%d (0x%x @ 0x%x)\r\n", continuous_page_count, pages, starting_page_dir, starting_page_off, *pageptr, pageptr);
      if(continuous_page_count>=pages) break;
    }
  }

  if(continuous_page_count<pages) {
    kprintf("    ERROR Found %l pages but need %l\r\n", continuous_page_count, pages);
    return NULL;
  }

  //now map the pages
  i = starting_page_dir;
  j = starting_page_off;
  uint32_t *pagedir_ptr = (uint32_t *)((vaddr)flat_pagetables_ptr + (vaddr)(starting_page_dir << 12) + (vaddr)(starting_page_off * sizeof(uint32_t)));
  kprintf("DEBUG vm_map_next_unallocated_pages starting allocation from 0x%x\r\n", pagedir_ptr);

  for(p=0;p<pages;p++) {
    pagedir_ptr[p] = ((vaddr)phys_addr[p] & MP_ADDRESS_MASK) | MP_PRESENT | flags;
    // pagedir_entry_vptr = ((vaddr *)pagedir_entries)[i];
    // if(!pagedir_entry_vptr) k_panic("ERROR vm_map_next_unallocated_pages page was present in physical map but not in virtual\r\n");

    // pagedir_entry_vptr[j] = ((vaddr)phys_addr[p] & MP_ADDRESS_MASK) | MP_PRESENT | flags;
    // j++;
    // if(j>1024) {
    //   i++;
    //   j=0;
    // }
  }

  //now calculate the virtual pointer
  void *ptr = (void *)(((vaddr)starting_page_dir) << 22 | ((vaddr)starting_page_off) << 12);

  //we can't zero the memory here because ptr might be in a different paging directory.
  return ptr;
}

/**
internal function to resolve a virtual memory pointer into a directory and page
location.
Arguments:
- vmem_ptr - pointer to resolve
- dir - pointer to 16-bit integer to receive the directory index
- off - pointer to a 16-bit integer to receive the offset within the given directory
*/
uint8_t _resolve_vptr(void *vmem_ptr, uint16_t *dir, uint16_t *off)
{
  size_t dir_idx = (size_t)vmem_ptr >> 22;
  size_t dir_off = (size_t)vmem_ptr >> 12 & 0x03FF;

  *dir = (uint16_t)dir_idx;
  *off = (uint16_t)dir_off;
  return 1;
}

/**
unmaps the given pages, and marks them as free.
FIXME: should also wipe the contents for security.
Arguments:
- root_page_dir - paging directory to use. Pass NULL to use the kernel's one.
- vmem_ptr - virtual memory pointer to deallocate, relative to the given page directory. This must be 4k aligned, i.e.
begin on a page boundary.
- page_count - number of contigous pages to deallocate
*/
void vm_deallocate_physical_pages(uint32_t *root_page_dir, void *vmem_ptr, size_t page_count)
{
  int16_t dir,off;
  vaddr phys_ptr;
  size_t phys_page_idx;

  if(root_page_dir==NULL) root_page_dir = (uint32_t *)kernel_paging_directory;

  vaddr pageptr_offset = (vaddr)vmem_ptr >> 12;

  for(uint32_t p=pageptr_offset; p<pageptr_offset*page_count; p++) {
    flat_pagetables_ptr[p] = 0;
    __invalidate_vptr((vaddr)vmem_ptr+(p-pageptr_offset)*1024);

  }
  // _resolve_vptr(vmem_ptr, &dir, &off);
  // volatile uint32_t *pagedir_ent_phys = root_page_dir[dir] & MP_ADDRESS_MASK;
  // volatile uint32_t *pagedir_ent = k_map_if_required(root_page_dir, pagedir_ent_phys, MP_READWRITE);

  // for(register size_t i=0; i<page_count; i++) {
  //   mb();
  //   phys_ptr = (vaddr)pagedir_ent[off] & MP_ADDRESS_MASK;
  //   if(phys_ptr==0) {
  //     kprintf("ERROR deallocating ptr 0x%x from root dir 0x%x page %l phys_ptr is null\r\n", vmem_ptr, root_page_dir, i+off);
  //     continue;
  //   }
  //   pagedir_ent[off] = (uint32_t)0;
  //   vaddr vptr_to_invalidate = dir*0x400000 + off*0x1000;

  //   __invalidate_vptr(vptr_to_invalidate);

  //   phys_page_idx = phys_ptr >> 12;
  //   physical_memory_map[phys_page_idx].in_use = 0;
  //   off++;
  //   if(off>=1024) {
  //     off = 0;
  //     dir++;
  //     pagedir_ent_phys = root_page_dir[dir];
  //     pagedir_ent = k_map_if_required(root_page_dir, pagedir_ent_phys, MP_READWRITE);
  //   }
  // }
  mb();
}

void k_unmap_page_ptr(uint32_t *root_page_dir, void *vptr)
{
  size_t idx = ADDR_TO_PAGEDIR_IDX(vptr);
  size_t off = ADDR_TO_PAGEDIR_OFFSET(vptr);
  //kprintf("DEBUG k_unmap_page_ptr 0x%x corresponds to index %l, offset %l\r\n", vptr, idx, off);
  k_unmap_page(root_page_dir, idx, off);
}

/**
unmap a page of physical memory by removing it from the page table and setting the
page to "not present"
*/
void k_unmap_page(uint32_t *root_page_dir, uint16_t pagedir_idx, uint16_t pageent_idx)
{
  if(pagedir_idx>1023) return;  //not currently supported!
  if(pageent_idx>1023) return; //out of bounds

  if(root_page_dir==NULL) root_page_dir = kernel_paging_directory;

  // uint32_t *pagedir_ent_phys = (vaddr)root_page_dir[pagedir_idx] & MP_ADDRESS_MASK;
  // uint32_t *pagedir_ent;

  // if(pagedir_ent_phys==NULL) {
  //   kprintf("ERROR cannot unmap page %d-%d from root dir 0x%x as it is not present\r\n",pagedir_idx, pageent_idx, root_page_dir);
  //   return;
  // }

  // if(root_page_dir==kernel_paging_directory) {
  //   uint32_t *pagedir_entries = (uint32_t *)PAGEDIR_ENTRIES_LOCATION;
  //   pagedir_ent = pagedir_entries[pageent_idx];
  //   if(!pagedir_ent) {
  //     kprintf("ERROR k_unmap_page page directory %d shown as present in root dir but no vptr in cache table\r\n", pageent_idx);
  //     return;
  //   }
  // } else {
  //   pagedir_ent = k_map_if_required(NULL, pagedir_ent_phys, MP_READWRITE);
  // }
  // pagedir_ent[pageent_idx] = 0;
  uint32_t* page_ptr = (uint32_t *)((vaddr)flat_pagetables_ptr + (vaddr)pagedir_idx << 12 + (vaddr)pageent_idx*sizeof(uint32_t));
  kprintf("DEBUG k_unmap_page page for %d - %d is at 0x%x\r\n", pagedir_idx, pageent_idx, (vaddr)page_ptr);
  *page_ptr = 0;
}

/**
allocates the given number of pages and maps them into the memory space of the given page directory.
*/
void *vm_alloc_pages(uint32_t *root_page_dir, size_t page_count, uint32_t flags)
{
  if(page_count>512) return NULL; //for the time being only allow block allocation up to 512 pages.

  if(root_page_dir==NULL) root_page_dir = kernel_paging_directory;
  void *phys_ptrs[512];

  uint32_t allocd = allocate_free_physical_pages(page_count, (void **)&phys_ptrs);
  kprintf("  DEBUG allocated %d free physical pages\r\n", allocd);
  if(allocd<page_count) {
    kputs("  DEBUG insufficient pages allocd, deallocating and removing\r\n");
    deallocate_physical_pages(allocd,(void **) &phys_ptrs);
    return NULL;
  }

  mb();

  kprintf("  DEBUG mapping pages into vram\r\n");
  void* vmem_ptr = vm_map_next_unallocated_pages(root_page_dir, flags, phys_ptrs, page_count);
  if(vmem_ptr==NULL) {
    kprintf("  DEBUG mapping failed, deallocating physical pages and returning\r\n");
    deallocate_physical_pages(allocd, (void **)&phys_ptrs);
    return NULL;
  }
  return vmem_ptr;
}

// void* vm_find_existing_mapping(uint32_t *base_directory_vptr, void *phys_addr, size_t recursion_level)
// {
//   //save the offset within the page
//   uint32_t page_offset = (uint32_t) phys_addr & ~MP_ADDRESS_MASK;
//   //now search the page directory to try and find the physical address
//   uint32_t page_value = (uint32_t) phys_addr & MP_ADDRESS_MASK;

//   kprintf("DEBUG vm_find_existing_mapping looking for 0x%x in base dir 0x%x at level %d\r\n", phys_addr, base_directory_vptr, recursion_level);
//   // if(recursion_level>1024) {
//   //   k_panic("ERROR vm_find_existing_mapping excessive recursion detected\r\n");
//   // }

//   // if(base_directory_vptr==NULL) {
//   //   base_directory_vptr = (uint32_t *)kernel_paging_directory;
//   // }

//   // for(register int i=0;i<1023;i++) {
//   //   if(! (base_directory_vptr[i] & MP_PRESENT)) continue;  //don't bother searching empty directories

//   //   uint32_t *pagedir_entry_phys = (uint32_t *)(base_directory_vptr[i] & MP_ADDRESS_MASK);
//   //   if(pagedir_entry_phys==NULL) {
//   //     kprintf("WARNING unexpected present but nil pointer in directory entry %d\r\n", i);
//   //     continue;
//   //   }
//   //   uint32_t *pagedir_entry_vptr;
//   //   if((vaddr)pagedir_entry_phys < 0x100000) {
//   //     pagedir_entry_vptr = pagedir_entry_phys;  //this is identity-mapped
//   //   } else {
//   //     uint32_t *pagedir_entries = (uint32_t *)PAGEDIR_ENTRIES_LOCATION;
//   //     pagedir_entry_vptr = pagedir_entries[i];
//   //     if(!pagedir_entry_vptr) {
//   //       kprintf("WARNING page directory %d shown as present in root dir but no vptr in cache table\r\n", i);
//   //       continue;
//   //     }
//   //   }

//   //   for(register int j=0;j<1023;j++) {

//   //     if( (pagedir_entry_vptr[j] & MP_ADDRESS_MASK) == page_value) {
//   //       vaddr v_addr = (vaddr)((i << 22)+ (j << 12)) + (vaddr)page_offset;
//   //       return (void *)v_addr;
//   //     }
//   //   }
//   // }


//   for(uint32_t p=0;p<1024*1024;p++) {

//   }
//   return NULL;
// }

/**
if the given physical address is already mapped, then returns the virtual address
that it is mapped to.  If not then it finds an unallocated page, maps it and
returns that.
Arguments:
- base_directory_vptr - pointer (in kernel vmem) to the base virtual memory directory to use. If NULL then uses the kernel one.
- phys_addr           - pointer to the physical memory block to map
- flags               - extra MP_ flags to apply
Returns:
- a virtual memory pointer to the mapped block, or NULL on failure
*/
// void* k_map_if_required(uint32_t *base_directory_vptr, void *phys_addr, uint32_t flags)
// {
//   if(phys_addr==NULL) {
//     kprintf("WARNING k_map_if_required request to map NULL pointer\r\n");
//     return NULL;
//   }

//   if(base_directory_vptr==NULL) base_directory_vptr = kernel_paging_directory;

//   kprintf("DEBUG k_map_if_required request to map 0x%x into base dir at vptr 0x%x\r\n", phys_addr, base_directory_vptr);
//   //save the offset within the page
//   uint32_t page_offset = (uint32_t) phys_addr & ~MP_ADDRESS_MASK;
//   //now search the page directory to try and find the physical address
//   uint32_t page_value = (uint32_t) phys_addr & MP_ADDRESS_MASK;

//   kprintf("DEBUG k_map_if_required page_offset 0x%x page_value 0x%x\r\n", page_offset, page_value);

//   void* existing_mapping = vm_find_existing_mapping(base_directory_vptr, page_value, 0);
//   if(existing_mapping) return existing_mapping;


//   int16_t dir=0;
//   int16_t off=1;

//   //kprintf("DEBUG k_map_if_required no existing mapping found for 0x%x, creating a new one\r\n", phys_addr);
//   if(find_next_unallocated_page(base_directory_vptr, &dir,&off)==0) {
//     //allocation worked
//     void *mapped_page_addr = k_map_page(base_directory_vptr, (void *)page_value, dir, off, flags);
//     return mapped_page_addr + page_offset;
//   } else {
//     kprintf("ERROR Could not allocate any more virtual RAM in base directory %x\r\n", base_directory_vptr);
//     return NULL;
//   }
// }

/**
Finds the next unallocated page in virtual memory.
Arguments:
- base_directory_vptr - pointer to the base virtual memory directory to use.
- dir                 - pointer to directory offset at which to start searching base_directory. On successful return, this will contain the directory to map.
- off                 - page offset within the given directory at which to start searching
Returns:
0 if successful, 1 on failure.
*/
// uint8_t find_next_unallocated_page(uint32_t *base_directory_vptr, int16_t *dir, int16_t *off)
// {
//   register int16_t j=*off;
//   vaddr *pagedir_entries = (vaddr *)PAGEDIR_ENTRIES_LOCATION;

//   for(register int16_t i=*dir; i<1024; i++) {
//     if(! (base_directory_vptr[i] & MP_PRESENT)) continue;

//     uint32_t *directory_vptr = pagedir_entries[i] & MP_ADDRESS_MASK;
//     if(!directory_vptr) k_panic("ERROR find_next_unallocated_page, page directory entry was present in physical map but not in virtual map\r\n");
//     while(j<1024) {
//       if(! (directory_vptr[j] & MP_PRESENT)) {
//         *dir = i;
//         *off = j;
//         return 0;
//       }
//       ++j;
//     }
//     j=0;
//   }

//   return 1;
// }

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

/**
internal function called from apply_memory_map_protections to protect a given
memory block
*/
void _reserve_memory_block(size_t base_addr, uint32_t page_count) {
  size_t base_phys_page = ADDR_TO_PAGEDIR_IDX(base_addr);

  //kprintf("     Protecting %d pages from %x\r\n", page_count, base_phys_page);

  for(register size_t i=0;i<page_count;i++) {
    physical_memory_map[i+base_phys_page].in_use=1;
  }
}

/**
sets the "readonly" and "dirty" flag on the protected memory regions
*/
void apply_memory_map_protections(struct BiosMemoryMap *ptr)
{
  uint8_t entry_count = ptr->entries;
  //kputs("Applying memory protections\r\n");

  for(register int i=0;i<entry_count;i++){
    struct MemoryMapEntry *e = (struct MemoryMapEntry *)&ptr[2+i*24];
    uint32_t page_count = e->length / 4096;

    switch(e->type) {
      case MMAP_TYPE_RESERVED:
      case MMAP_TYPE_ACPI_RECLAIMABLE:
      case MMAP_TYPE_ACPI_NONVOLATILE:
      case MMAP_TYPE_BAD:
        _reserve_memory_block((size_t)e->base_addr, page_count);
        break;
      default:
        break;
    }
  }
}

void allocate_physical_map(struct BiosMemoryMap *ptr)
{
  register int i;
  //find the highest value in the physical memory map. We must allocate up to here.
  uint8_t entry_count = ptr->entries;

  size_t highest_value = 0;

  //kprintf("DEBUG allocate_physical_map ptr is 0x%x, entry count is %d\r\n", ptr, entry_count);

  for(i=0;i<entry_count;i++){
    struct MemoryMapEntry *e = (struct MemoryMapEntry *)&ptr[2+i*24];
    size_t entry_limit = (size_t) e->base_addr + (size_t)e->length;
    if(entry_limit>highest_value) highest_value = entry_limit;
  }

  physical_page_count = highest_value / PAGE_SIZE;  //4k pages

  kprintf("Detected %d Mb of physical RAM in %d pages\r\n", highest_value/1048576, physical_page_count);

  //how many of our map entries fit in a 4k page?
  size_t entries_per_page = PAGE_SIZE / (size_t) sizeof(struct PhysMapEntry);
  size_t pages_to_allocate = physical_page_count / entries_per_page + 1;

  //since we don't have a physical map yet, we are going to have to rely on some
  //intuition and prerequisites here.....
  //1. We assume that the whole first Mb is identity-mapped
  //2. We assume that the kernel itself (code and static data) fits within 16 pages (64k)
  //3. We assume a standard memory layout, i.e. the kernel is loaded from 0x7e00 (page 0x07)
  //and that there is a memory hole at 0x80000 (page 0x80=128)
  //Therefore, we can start out pseudo-allocation from page 24 (=0x18) but can't go beyond 0x80 - limit of 0x5c = 92 pages
  //Given each page takes up 1 byte each page should be able to take 4096 of them

  kprintf("DEBUG %d map entries per 4k page\r\n", entries_per_page);
  kprintf("Allocating %d pages to physical ram map\r\n", pages_to_allocate);
  if(pages_to_allocate>92) {
    kprintf("Needed to allocate %d pages for physical memory map but have a limit of 92\r\n");
    k_panic("Unable to allocate physical memory map\r\n");
  }

  physical_memory_map = (struct PhysMapEntry *)0x18000;
  for(i=0;i<physical_page_count;i++) {
    physical_memory_map[i].present=1;
    physical_memory_map[i].in_use=0;
  }

  /**
  initial populate of the physical memory map. We use the "dirty" flag in the physical memory
  map to indicate "in use".
  Pages 0x7->0x15  (0x7000  -> 0x15000) are reserved for kernel
  Pages 0x80->0xff(0x80000 -> 0x100000) are reserved for bios
  */
  physical_memory_map[0].in_use=1;                        //always protect first page, reserved for SMM
  size_t dir_page = ROOT_PAGE_DIR_LOCATION >> 12;
  physical_memory_map[dir_page].in_use=1;
  dir_page = FIRST_PAGEDIR_ENTRY_LOCATION >> 12;
  physical_memory_map[dir_page].in_use=1;

  for(i=7;i<=0x15;i++) physical_memory_map[i].in_use=1;    //kernel memory, incl. initial paging directories
  for(i=0x70;i<0x80;i++) physical_memory_map[i].in_use=1;  //kernel stack
  for(i=0x80;i<=0xFF;i++) physical_memory_map[i].in_use=1; //standard BIOS / hw area
  for(i=0x18;i<pages_to_allocate+0x18;i++) physical_memory_map[i].in_use=1; //physical memory map itself
}

uint32_t *initialise_app_pagingdir(void *root_dir_phys, void *page_one_phys)
{
  uint32_t *app_paging_dir_root_kmem = k_map_next_unallocated_pages(MP_PRESENT|MP_READWRITE, &root_dir_phys, 1);

  app_paging_dir_root_kmem[0] = (uint32_t)page_one_phys | MP_PRESENT | MP_READWRITE; //we need kernel-only access to page 0 so that we can use interrupts

  //copy over the content of the first page of the kernel paging directory to the application paging directory
  uint32_t *page_one = (uint32_t *)k_map_next_unallocated_pages(MP_PRESENT|MP_READWRITE, &page_one_phys, 1);

  memcpy_dw(page_one, FIRST_PAGEDIR_ENTRY_LOCATION, PAGE_SIZE/8);

  //this page contains the IDT
  page_one[1] = (1 << 12) | MP_PRESENT | MP_USER;       //user-mode needs readonly in order to access IDT

  mb();
  return app_paging_dir_root_kmem;
}

vaddr _mmgr_get_pd();

/**
 * This is called from the pagefault handler and it will allocate extra paging directories as required, provided that the fault occurred from
 * kernel code when accessing the page mapped area.
 * 
 * Returns 0 if the exception was handled and 1 otherwise.
*/
uint8_t handle_allocation_fault(uint32_t pf_load_addr, uint32_t error_code, uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags)
{
  void *phys_ptr;
  if((vaddr)pf_load_addr >= (vaddr)flat_pagetables_ptr && (vaddr)pf_load_addr <= (vaddr)flat_pagetables_ptr+0x100000) { //the fault occurred in the mapped page-tables range
    kputs("DEBUG handle_allocation_fault detected in pagetables area\r\n");
    if(faulting_codeseg!=0x08 || error_code&PAGEFAULT_ERR_USER) {
      kputs("DEBUG the fault occurred in user code, not handling.\r\n");
      return 1;
    }
    if(error_code&PAGEFAULT_ERR_PRESENT) {  //if not set, caused by page-not-present
      kputs("DEBUG error was a protection violation, not handling\r\n");
      return 1;
    }

    //OK, so we need to allocate a page table entry. Work out where it goes.
    vaddr pf_loc = (vaddr)pf_load_addr - (vaddr) flat_pagetables_ptr;
    vaddr pagedir_idx = pf_loc >> 12;
    vaddr pagedir_ent = (pf_loc >> 2) & 0x3FF;
    kprintf("DEBUG allocating pagedir at %l-%l from 0x%x\r\n", pagedir_idx, pagedir_ent, pf_loc);
    uint32_t *current_pd = (uint32_t *)_mmgr_get_pd();
    kprintf("DEBUG current PD is 0x%x\r\n", current_pd);
    if(current_pd!=kernel_paging_directory) {
      kputs("ERROR JIT allocation to non-kernel directories is not implemented yet\r\n");
      return 1;
    }

    //this code only works for the kernel PD as it's identity-mapped
    //need to get some kind of translation table for others
    uint32_t allocd = allocate_free_physical_pages(1, &phys_ptr);
    if(allocd!=1) {
      kputs("ERROR Could not allocate more physical RAM\r\n");
      return 1;
    }
    kprintf("DEBUG physical ptr to map is 0x%x\r\n", phys_ptr);

    //set up the new page in the root paging directory.
    current_pd[pagedir_idx] = ((vaddr)phys_ptr & MP_ADDRESS_MASK) | MP_PRESENT | MP_READWRITE;

    //also, map it into RAM at the right place in the page-tables area.  For that we need to get a pointer
    //to the page entry holding the page-tables area.

    vaddr flat_pagetables_ptr_pageoff = (vaddr)flat_pagetables_ptr >> 10;
    uint32_t* pagetables_entry = (uint32_t *)((vaddr)flat_pagetables_ptr + flat_pagetables_ptr_pageoff);
    pagetables_entry[pagedir_idx] = (vaddr)phys_ptr | MP_PRESENT | MP_READWRITE;
    kprintf("DEBUG handle_allocation_fault flat pagetables at 0x%x\r\n", flat_pagetables_ptr);
    kprintf("DEBUG pagetables address of pagetables is 0x%x\r\n", pagetables_entry);

    //the faulting operation can now be retried by returning 0
    return 0;
  } else {  //not relevant to us.
    return 1;
  }
}

/**
 * internal function to return the physical address of the current paging directory
*/
vaddr _mmgr_get_pd()
{
  vaddr result;

  asm("mov %%cr3, %0" : "=r"(result) : );
  return result;
}