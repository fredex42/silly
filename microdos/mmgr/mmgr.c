#include <types.h>
#include <memops.h>
#include <errors.h>
#include "mmgr.h"
#include "../utils/v86.h"
#include "../utils/tss32.h"
#include "../utils/memlayout.h"
#include "../utils/regstate.h"

static struct MemoryMapEntry *memoryMap;  //the BIOS memory map returned by int 15
static struct PhysMapEntry *physicalMap;  //the physical memory map that we maintain
static uint32_t memory_map_entry_count;
static size_t physical_page_count;

uint16_t retrieve_memory_map()
{
    memory_map_entry_count = 0;
    memoryMap = (struct MemoryMapEntry *) MEMORY_MAP_STATICPTR;
    memset(memoryMap, 0, MEMORY_MAP_LIMIT);

    uint16_t intseg=0,intoff=0;
    uint32_t returned_flags=0, returned_bx=0, returned_ax=0;

    activate_v86_tss();
    do {
        get_realmode_interrupt(0x15, &intseg, &intoff);
        prepare_v86_call(intseg, intoff, "lea .mem_call_rtn,%%eax\n");

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
        kprintf("returned from v86, eax=0x%x, ebx=0x%x flags=0x%x\r\n", returned_ax, returned_bx, returned_flags);
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
  physicalMap = (struct PhysMapEntry *)physical_map_start;

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
  for(register uint32_t i=0; i<0x78; i++) {
    uint32_t new_addr = (uint32_t) CLASSIC_ADDRESS(0, i+7);
    new_pagedir[i] = new_addr | MP_PRESENT | MP_GLOBAL;
  }
  return ERR_NONE;
}

/**
 * Scans the physical memory map to try to find a block of pages that are free
 * @params:
 *  optional_start_addr - start scanning from this address. Default to 0, meaning start from the beginning of the map
 * optional_end_addr - end scanning at this address. Default to 0, meaning end at the end of the map
 */
err_t find_next_free_pages(uint32_t optional_start_addr, uint32_t optional_end_addr, uint32_t page_count, uint8_t should_reverse, uint32_t *out_block_start)
{
  uint32_t start_index = CLASSIC_PAGE_INDEX(optional_start_addr);
  uint32_t end_index = end_index>0 ? CLASSIC_PAGE_INDEX(optional_end_addr) : physical_page_count;

  register uint32_t i = should_reverse ? end_index : start_index;
  register uint32_t found_pages = 0;
  uint32_t starting_page = 0;

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
  uint32_t *level_one_tables = (uint32_t *)(( (physical_map_start - (physical_page_count*4)) ) & MP_ADDRESS_MASK);  //4 bytes per physical page entry
  
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