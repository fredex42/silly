#include <types.h>
#include <memops.h>
#include <errors.h>
#include <panic.h>
#include "mmgr.h"
#include "../utils/v86.h"
#include "../utils/tss32.h"
#include "../utils/memlayout.h"
#include "../utils/regstate.h"

struct MemoryMapEntry *memoryMap;  //the BIOS memory map returned by int 15
//struct PhysMapEntry *physicalMap;  //the physical memory map that we maintain
uint32_t memory_map_entry_count;
size_t physical_page_count;
uint32_t *level_one_tables;

uint16_t retrieve_memory_map()
{
    memory_map_entry_count = 0;
    level_one_tables = NULL;
    memoryMap = (struct MemoryMapEntry *) MEMORY_MAP_STATICPTR;
    memset(memoryMap, 0, MEMORY_MAP_LIMIT);

    uint16_t intseg=0,intoff=0;
    uint32_t returned_flags=0, returned_bx=0, returned_ax=0;

    activate_v86_tss();
    do {
        get_realmode_interrupt(0x15, &intseg, &intoff);
        prepare_v86_call(intseg, intoff, "lea .mem_call_rtn,%%eax\n", 0);

        asm __volatile__(
            //find the right destination pointer
            "mov %4, %%ecx\n"   //current count
            "mov $24, %%eax\n"    //size of a map entry
            "mul %%ecx\n"       //eax = eax * ecx
            "mov memoryMap, %%edi\n"
            "add %%eax, %%edi\n"    //edi = edi + eax
            //set up magic numbers for the call
            "xor %%eax, %%eax\n"
            "mov %%ax, %%es\n"
            "mov $0xe820, %%eax\n"
            "mov $0x534d4150, %%edx\n"
            "mov %0, %%ebx\n"
            "mov $24, %%ecx\n"
            "iret\n"
            ".mem_call_rtn:\n"
            "int $0xff\n" //exit v86 mode
            "mov %%ebx, %1\n"   //retrieve bx value (next offset)
            "mov %%eax, %2\n"   //retrieve ax value (error code)
            "pushf\n"
            "pop %%eax\n"
            "mov %%eax, %3\n"   //retrieve eflags value
            : "+m" (returned_bx)
            : "m" (returned_bx), "m"(returned_ax), "m"(returned_flags), "m"(memory_map_entry_count)
            : "eax", "ebx", "ecx", "edx", "edi"
        );
        if((returned_flags & 0x01) && returned_ax==0x86) {
            kputs("WARNING BIOS does not support enhanced memory-map retrieval\r\n");
            return ERR_NOTSUPP;
        } else if(returned_flags & 0x01) {
            kprintf("WARNING Unable to retrieve memory-map, BIOS error was 0x%x\r\n", returned_ax);
            return ERR_UNKNOWN;
        }
        //kprintf("returned from v86, eax=0x%x, ebx=0x%x flags=0x%x\r\n", returned_ax, returned_bx, returned_flags);
        memory_map_entry_count += 1;
    } while(!(returned_flags&0x3) || returned_bx!=0);
    return ERR_NONE;
}

void dump_memory_map() {
    kprintf("Detected memory map has %d entries:\r\n", memory_map_entry_count);
    for(register int i=0;i<memory_map_entry_count;i++){
      struct MemoryMapEntry e = memoryMap[i];
      uint32_t page_count = e.length / 4096;
      kprintf("%x | %x | %d: ", (size_t)e.base_addr, (size_t)e.length, page_count);
      switch (e.type) {
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

void initialise_mmgr() {
  struct PhysMapEntry **physicalMap = (struct PhysMapEntry **)PHYSICAL_MAP_PTR;

  size_t physical_map_start = 0;
  size_t physical_map_length_in_pages = 0;
  uint32_t *root_paging_dir = 0;
  size_t pages_used_for_paging = 0;

  kputs("Initialising memory manager...\r\n");
  retrieve_memory_map();
  dump_memory_map();
  size_t highest_value = highest_free_memory();
  kputs("Allocating physical map space...\r\n");
  allocate_physical_map(highest_value, &physical_map_start, &physical_map_length_in_pages);
  *physicalMap = (struct PhysMapEntry *)physical_map_start;

  allocate_paging_directories(highest_value, physical_map_start, &root_paging_dir, &pages_used_for_paging);
  kprintf("Used %d pages for paging\r\n", pages_used_for_paging);
  kprintf("Root paging directory is at 0x%x\r\n", root_paging_dir);
  setup_physical_map(CLASSIC_PAGE_INDEX(root_paging_dir), pages_used_for_paging);
  activate_paging(root_paging_dir);
  kputs("Activated paging\r\n");

  debug_dump_used_memory();
}

/**
 * Scans the BIOS memory map to find the highest address of free memory
*/
size_t highest_free_memory()
{
  size_t highest_range_end = 0;

  for(register int i=0;i<memory_map_entry_count;i++) {
    struct MemoryMapEntry e = memoryMap[i];
    switch(e.type) {
      case MMAP_TYPE_USABLE:
        highest_range_end = (size_t)e.base_addr + (size_t)e.length;
        break;
      default:
        break;
    }
  }
  return highest_range_end;
}

void allocate_physical_map(size_t highest_value, size_t *area_start_out, size_t *map_length_in_pages_out)
{
  physical_page_count = highest_value / PAGE_SIZE;
  kprintf("Detected %d Mb of physical RAM in %d pages\r\n", highest_value/1048576, physical_page_count);
  kprintf("End of usable RAM at 0x%x\r\n", highest_value);
  //how many of our map entries fit in a 4k page?
  size_t entries_per_page = PAGE_SIZE / (size_t) sizeof(struct PhysMapEntry);
  size_t pages_to_allocate = physical_page_count / entries_per_page + 1;

  //The physical RAM map is allocated at the end of physical RAM (wherever that is)
  //and then gets mapped towards the end of VRAM
  kprintf("DEBUG %d map entries per 4k page\r\n", entries_per_page);
  kprintf("Allocating %d pages to physical ram map\r\n", pages_to_allocate);
  *map_length_in_pages_out = pages_to_allocate;
  size_t physical_map_start = highest_value - ((pages_to_allocate+1) * 0x1000);
  kprintf("Physical memory map runs from 0x%x to 0x%x\r\n", physical_map_start, highest_value);
  *area_start_out = physical_map_start;
}

void debug_dump_used_memory() 
{
  struct PhysMapEntry **physicalMapPtr = (struct PhysMapEntry **)PHYSICAL_MAP_PTR;
  struct PhysMapEntry *physicalMap = *physicalMapPtr;

  uint8_t in_use=1; //we know that page 0 is in use
  uint32_t region_start = 0;
  uint32_t ctr = 0;

  for(register uint32_t i=0; i<=physical_page_count; i++) {
    if(!physicalMap[i].in_use) {
      if(in_use || i>=physical_page_count) {
        //we just toggled
        uint32_t start_addr = CLASSIC_ADDRESS_FROM_INDEX(region_start);
        uint32_t end_addr = CLASSIC_ADDRESS_FROM_INDEX(i);
        kprintf("Region %d: 0x%x -> 0x%x\r\n", ctr, start_addr, end_addr);
        in_use = 0;
        region_start = 0;
        ++ctr;
      }
    } else if(!in_use) {
      region_start = i;
      in_use = 1;
    }
  } 

  if(region_start!=0) {
    kprintf("Region %d: 0x%x -> 0x%x\r\n", ctr, CLASSIC_ADDRESS_FROM_INDEX(region_start), CLASSIC_ADDRESS_FROM_INDEX(physical_page_count));
  }
}

/**
 * Initialise the physical memory map, by reading the BIOS provided information and marking only 'free'
 * areas as both present and available.
 */
void setup_physical_map(uint32_t first_page_of_pagetables, size_t pages_used_for_paging)
{
  struct PhysMapEntry **physicalMapPtr = (struct PhysMapEntry **)PHYSICAL_MAP_PTR;
  struct PhysMapEntry *physicalMap = *physicalMapPtr;
  uint32_t bios_map_idx = 0;

  for(register uint32_t i=0; i<=physical_page_count; i++) {
    if(CLASSIC_ADDRESS_FROM_INDEX(i) < memoryMap[bios_map_idx].base_addr) {
      //if we have a hole in the memory, mark it as not present
      physicalMap[i].present = 0;
    } else {
      physicalMap[i].present = 1;
      //mark any bad, reserved or ACPI ranges as in-use.  ACPI ranges can be freed later.
      if(memoryMap[bios_map_idx].type!=MMAP_TYPE_USABLE) {
        physicalMap[i].in_use = 1;
      }
    }

    if(CLASSIC_ADDRESS_FROM_INDEX(i)>(memoryMap[bios_map_idx].base_addr+memoryMap[bios_map_idx].length)) {
      ++bios_map_idx;
    }
    if(bios_map_idx > memory_map_entry_count) {
      kprintf("WARNING: 0x%x > 0x%x\r\n", bios_map_idx, memory_map_entry_count);
      break;
    }
  }

  //also, we need to mark the pages we already have for the kernel. We reserve pages 7 -> 7F
  for(register uint32_t i=7; i<0x80; i++) {
    physicalMap[i].in_use = 1;
  }
  //and page 0 which has BIOS stuff on it
  physicalMap[0].in_use = 1;

  //and the physical map itself....
  uint32_t phys_map_len = ((physical_page_count * sizeof(struct PhysMapEntry)) / PAGE_SIZE) + 1;  //extra page to allow for over-runs
  kprintf("Physical map length is 0x%x pages\r\n", phys_map_len);
  for(register uint32_t i=physical_page_count-1; i<=(physical_page_count - phys_map_len); i--) {
    physicalMap[i].in_use = 1;
  }
  
  //and the paging directories...
  for(register uint32_t i=0; i<=pages_used_for_paging; i++) {
    physicalMap[i+first_page_of_pagetables].in_use = 1;
  }
}

/**
 * Relocates the kernel code from conventional memory to the requested base address
 * Returns the number of extra pages used for the paging directories.
 * 
 */
err_t relocate_kernel(uint32_t new_base_addr) {
  kprintf("Relocating kernel to 0x%x\r\n", new_base_addr);
  //assume kernel is on memory pages 0x07-0x7F
  uint32_t base_pagetable = CLASSIC_PAGETABLE(new_base_addr);
  uint32_t base_pagedir = CLASSIC_PAGEDIR(new_base_addr);

  uint32_t *new_pagedir;
  uint32_t *base_paging_dir = 0x0;

  //get the current paging dir
  asm (
    "mov %%cr3, %0"
    : "rm+"(base_paging_dir)
    : "rm"(base_paging_dir)
  );
  kprintf("DEBUG base_paging_dir is 0x%x\r\n", base_paging_dir);

  uint32_t used_pages = 0;

  if(base_paging_dir[base_pagedir] & MP_PRESENT) {
    //we do have a paging directory present, use that
    new_pagedir = (uint32_t *)(base_paging_dir[base_pagedir] & MP_ADDRESS_MASK);
  } else {
    //we do not have a paging directory present, let's add one.
    ++used_pages;
    err_t e = find_next_free_pages(0x10000, base_paging_dir, 1, 1, &new_pagedir);
    if(e!=ERR_NONE) {
      kprintf("ERROR Unable to find a free page: 0x%x\r\n", e);
      return e;
    }
    base_paging_dir[base_pagedir] = (uint32_t)new_pagedir | MP_PRESENT | MP_GLOBAL | MP_READWRITE;
  }
  for(register uint32_t i=7; i<0x80; i++) {
    uint32_t new_addr = (uint32_t) CLASSIC_ADDRESS(0, i);
    new_pagedir[i] = new_addr | MP_PRESENT | MP_GLOBAL;
  }
  return ERR_NONE;
}

/**
 * map a page from physical memory to virtual memory
 * 
 * Since most of the memory is indentity-mapped you don't need this much;
 * it's used by v86 routines to re-arrange the conventional RAM area
 * 
 * params:
 * - phys_addr - physical pointer to the memory to map
 * - virt_addr - virtual address to map it too
 * - flags - MP_ flags to use. MP_PRESENT is set for you.
 */
err_t k_map_page(void * phys_addr, void * virt_addr, uint16_t flags)
{
  uint32_t *base_paging_dir = 0x0;

  if((size_t)phys_addr & ~MP_ADDRESS_MASK) {
    k_panic("you must call k_map_page with the address of a page boundary (4k) to map\r\n");
  }
  if((size_t)virt_addr & ~MP_ADDRESS_MASK) {
    k_panic("you must call k_map_page with the address of a page boundary (4k) to map\r\n");
  }

  //get the current paging dir
  asm (
    "mov %%cr3, %0"
    : "rm+"(base_paging_dir)
    : "rm"(base_paging_dir)
  );
  kprintf("DEBUG k_map_page base_paging_dir is 0x%x\r\n", base_paging_dir);

  size_t offset = (size_t)virt_addr >> 12;
  kprintf("DEBUG k_map_page offset is 0x%x\r\n", offset);
  uint32_t *ptr = (uint32_t *)((size_t)base_paging_dir +PAGE_SIZE+ (offset*sizeof(uint32_t)));
  kprintf("DEBUG k_map_page ptr is 0x%x\r\n", ptr);
  *ptr = (uint32_t)phys_addr | MP_PRESENT | flags;
  asm __volatile__("wbinvd"); //ensure that main memory is synced
  return ERR_NONE;
}

void k_unmap_page(void *virt_ptr)
{
  uint32_t *base_paging_dir = 0x0;

  if((size_t)virt_ptr & ~MP_ADDRESS_MASK) {
    k_panic("you must call k_unmap_page with the address of a page boundary (4k) to map\r\n");
  }

  //get the current paging dir
  asm (
    "mov %%cr3, %0"
    : "rm+"(base_paging_dir)
    : "rm"(base_paging_dir)
  );
  kprintf("DEBUG k_unmap_page base_paging_dir is 0x%x\r\n", base_paging_dir);
  

  uint16_t pd = CLASSIC_PAGEDIR(virt_ptr);
  uint16_t pt = CLASSIC_PAGETABLE(virt_ptr);
  uint32_t *pd_value = (uint32_t *)((size_t)base_paging_dir + pd*sizeof(uint32_t));
  uint32_t *ptr = (uint32_t *)(((uint32_t)(*pd_value) & MP_ADDRESS_MASK) + pt*sizeof(uint32_t));
  *ptr = 0;
  asm __volatile__("wbinvd"); //ensure that main memory is synced
}

/**
 * Looks up the physical page corresponding to the given address. 
 */
size_t virt_to_phys(void *virt_ptr, uint16_t *flags_out)
{
  uint32_t *base_paging_dir = 0x0;
  //get the current paging dir
  asm (
    "mov %%cr3, %0"
    : "rm+"(base_paging_dir)
    : "rm"(base_paging_dir)
  );
  kprintf("DEBUG virt_to_phys base_paging_dir is 0x%x\r\n", base_paging_dir);
  kprintf("DEBUG virt_to_phys vptr is 0x%x\r\n", virt_ptr);

  uint16_t pd = CLASSIC_PAGEDIR(virt_ptr);
  uint16_t pt = CLASSIC_PAGETABLE(virt_ptr);
  kprintf("DEBUG virt_to_phys page directory offset is 0x%x\r\n", pd);
  uint32_t *pd_value = (uint32_t *)((size_t)base_paging_dir + pd*sizeof(uint32_t));
  kprintf("DEBUG virt_to_phys page directory is at 0x%x and value is 0x%x\r\n", pd_value, *pd_value);
  uint32_t *ptr = (uint32_t *)(((uint32_t)(*pd_value) & MP_ADDRESS_MASK) + pt*sizeof(uint32_t));
  kprintf("DEBUG virt_to_phys page table entry is at 0x%x and value is 0x%x\r\n", ptr, *ptr);
  uint32_t value = *ptr;
  kprintf("DEBUG virt_to_phys pagetable value is 0x%x\r\n", value);
  if(flags_out != NULL) {
    *flags_out = value & ~MP_ADDRESS_MASK;
  }
  return value & MP_ADDRESS_MASK;
}

void unmap_kernel_boot_space()
{
  uint32_t *new_pagedir;
  uint32_t *base_paging_dir = 0x0;

  //get the current paging dir
  asm (
    "mov %%cr3, %0"
    : "rm+"(base_paging_dir)
    : "rm"(base_paging_dir)
  );
  kprintf("DEBUG base_paging_dir is 0x%x\r\n", base_paging_dir);
  uint32_t *pagetable_zero = (uint32_t *)(base_paging_dir[0] & MP_ADDRESS_MASK);
  for(register uint32_t i=7; i<0x7F; i++) {
    pagetable_zero[i] &= ~MP_PRESENT;  //remove the MP_PRESENT flag so the pages go
  }
}
/**
 * Scans the physical memory map to try to find a block of pages that are free
 * @params:
 *  optional_start_addr - start scanning from this address. Default to 0, meaning start from the beginning of the map
 * optional_end_addr - end scanning at this address. Default to 0, meaning end at the end of the map
 */
err_t find_next_free_pages(uint32_t optional_start_addr, uint32_t optional_end_addr, uint32_t page_count, uint8_t should_reverse, uint32_t *out_block_start)
{
  struct PhysMapEntry **physicalMapPtr = (struct PhysMapEntry **)PHYSICAL_MAP_PTR;
  struct PhysMapEntry *physicalMap = *physicalMapPtr;
  kprintf("DEBUG find_next_free_pages start_addr=0x%x end_addr=0x%x page_count=%d\r\n", optional_start_addr, optional_end_addr, page_count);
  uint32_t start_index = CLASSIC_PAGE_INDEX(optional_start_addr);
  uint32_t end_index = optional_end_addr>0 ? CLASSIC_PAGE_INDEX(optional_end_addr) : physical_page_count;

  register uint32_t i = should_reverse ? end_index : start_index;
  register uint32_t found_pages = 0;
  uint32_t starting_page = 0;

  kprintf("DEBUG find_next_free_pages start_index=0x%x end_index=0x%x\r\n", start_index, end_index);

  while(i<=end_index && i>=start_index) {
    if(physicalMap[i].in_use && found_pages>0) {
      //if this page is in use and we are already tracking pages, then there are not enough
      found_pages = 0;
      starting_page = 0;
    }
    if(!physicalMap[i].in_use) {
      ++found_pages;
      if(starting_page==0) starting_page = i;
    }
    if(found_pages==page_count) {
      //we found enough pages to return
      //Mark pages as in-use
      for(uint32_t j=starting_page; j<=i; j++) {
        physicalMap[j].in_use = 1;
      }
      
      *out_block_start = CLASSIC_ADDRESS_FROM_INDEX(starting_page);
      return ERR_NONE;
    }
    if(should_reverse) {
      i--;
    } else {
      i++;
    }
  }
  //we ran out of memory
  return ERR_NOMEM;
}

void allocate_paging_directories(size_t highest_value, size_t physical_map_start, uint32_t **root_paging_dir_out, size_t *pages_used_out)
{
  physical_page_count = highest_value / PAGE_SIZE;

  size_t level_one_table_count = physical_page_count;  //limit of 1024*1024 pages, IF we are in classic paging
  size_t level_two_dir_count = level_one_table_count / 1024;  //1024 level one dirs per level two, IF we are in classic paging
  *pages_used_out = level_two_dir_count + 1;  //extra +1 is the root directory

  //level one tables go below the physical memory map
  //Use MP_ADDRESS_MASK to make sure that this is page-aligned.
  level_one_tables = (uint32_t *)(( (physical_map_start - (physical_page_count*4)) ) & MP_ADDRESS_MASK);  //4 bytes per physical page entry
  
  kprintf("DEBUG %d level one entries from 0x%x\r\n", level_one_table_count, (uint32_t)level_one_tables);
  //identity map the entire RAM space
  for(register size_t i=0; i<=level_one_table_count; i++) {
    level_one_tables[i] = (i*PAGE_SIZE) | MP_PRESENT | MP_READWRITE | MP_USER;
  }

  //in classic paging, level two dirs must fit on a single page
  uint32_t *level_two_dirs = (uint32_t *)(((size_t)level_one_tables - (level_two_dir_count*4)) & MP_ADDRESS_MASK);  //4 bytes per level one table entry
  kprintf("DEBUG %d level two entries from 0x%x\r\n", level_two_dir_count, (uint32_t) level_two_dirs);
  for(register size_t i=0; i<=level_two_dir_count; i++) {
    uint32_t *lvl_one_ptr = &level_one_tables[i*1024];
    level_two_dirs[i] = (uint32_t)lvl_one_ptr | MP_PRESENT | MP_READWRITE | MP_USER;
  }
  *root_paging_dir_out = level_two_dirs;
}

void activate_paging(uint32_t *root_paging_dir)
{
  asm __volatile__(
    "mov %0, %%eax\n"
    "mov %%eax, %%cr3\n"
    "mov %%cr0, %%eax\n"
    "and $0x9FFEFFFF, %%eax\n"  //turn off bits 30 (cache disable), 29 (not write-through), 16 (write protect)
    "or $0x80000000, %%eax\n" //turn on bit 31 (paging)
    "mov %%eax, %%cr0\n"      //activate
    :
    : "rm"(root_paging_dir)
    : "eax"
  );
}

/**
allocates the given number of pages of physical RAM and maps them into the memory space of the given page directory.
*/
void *vm_alloc_pages(uint32_t *root_page_dir, size_t page_count, uint32_t flags)
{
  if(root_page_dir!=NULL) {
    k_panic("ERROR - allocating memory to alternative root dirs is not supported\r\n");
    return NULL;
  }
  uint32_t found_block_linear_start = 0;

  //only allocate out from 1Mb, we have continuously mapped memory there
  err_t e = find_next_free_pages(0x100000, 0, page_count, 0, &found_block_linear_start);
  if(e!=ERR_NONE) {
    kprintf("ERROR unable to allocate %l pages: 0x%x\r\n", page_count, e);
    return NULL;
  }

  //apply the requested flags. We can do this because all page directories are linearly mapped so we don't need to 
  //worry about switching directories.
  uint32_t block_page_start = found_block_linear_start >> 12;
  uint32_t block_page_end = block_page_start + page_count;
  for(register uint32_t i=block_page_start;i<block_page_end; i++) {
    level_one_tables[i] = (level_one_tables[i] & MP_ADDRESS_MASK) | flags | MP_PRESENT; //ensure MP_PRESENT is still thre
  }
  return (void *)found_block_linear_start;
}

void vm_deallocate_physical_pages(uint32_t *root_page_dir, void *phys_ptr, size_t page_count)
{
  struct PhysMapEntry **physicalMapPtr = (struct PhysMapEntry **)PHYSICAL_MAP_PTR;
  struct PhysMapEntry *physicalMap = *physicalMapPtr;
  uint32_t start_page_idx = CLASSIC_PAGE_INDEX(phys_ptr);
  for(register uint32_t i=start_page_idx; i<start_page_idx+page_count; i++) {
    physicalMap[i].in_use = 0;
  }
}