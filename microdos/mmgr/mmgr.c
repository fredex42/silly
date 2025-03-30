#include <types.h>
#include <memops.h>
#include <errors.h>
#include "mmgr.h"
#include "../utils/v86.h"
#include "../utils/tss32.h"
#include "../utils/memlayout.h"
#include "../utils/regstate.h"

static struct MemoryMapEntry *memoryMap;
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
  size_t root_paging_dir = 0;
  size_t pages_used_for_paging = 0;

  kputs("Initialising memory manager...\r\n");
  retrieve_memory_map();
  dump_memory_map();
  size_t highest_value = highest_free_memory();
  kputs("Allocating physical map space...\r\n");
  allocate_physical_map(highest_value, &physical_map_start, &physical_map_length_in_pages);
  allocate_paging_directories(highest_value, physical_map_start, &root_paging_dir, &pages_used_for_paging);
  kprintf("Used %d pages for paging\r\n", pages_used_for_paging);
  kprintf("Root paging directory is at 0x%x\r\n", root_paging_dir);
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

void allocate_paging_directories(size_t highest_value, size_t physical_map_start, size_t *root_paging_dir_out, size_t *pages_used_out)
{
  physical_page_count = highest_value / PAGE_SIZE;

  size_t level_one_table_count = physical_page_count;  //1024 pages per level one dir, IF we are in classic paging
  size_t level_two_dir_count = level_one_table_count / 1024;  //1024 level one dirs per level two, IF we are in classic paging
  *pages_used_out = level_two_dir_count + 1;  //extra +1 is the root directory

  //level one tables go below the physical memory map
  //Use MP_ADDRESS_MASK to make sure that this is page-aligned.
  uint32_t *level_one_tables = (uint32_t *)(( (physical_map_start - (physical_page_count*4)) ) & MP_ADDRESS_MASK);  //4 bytes per physical page entry
  
  kprintf("DEBUG %d level one entries from 0x%x\r\n", level_one_table_count, (uint32_t)level_one_tables);
  //identity map the entire RAM space
  for(register size_t i=0; i<level_one_table_count; i++) {
    level_one_tables[i] = (i*PAGE_SIZE) | MP_PRESENT | MP_READWRITE;
  }

  //in classic paging, level two dirs must fit on a single page
  uint32_t *level_two_dirs = (uint32_t *)(((size_t)level_one_tables - (level_two_dir_count*4)) & MP_ADDRESS_MASK);  //4 bytes per level one table entry
  kprintf("DEBUG %d level two entries from 0x%x\r\n", level_two_dir_count, (uint32_t) level_two_dirs);
  for(register size_t i=0; i<level_two_dir_count; i++) {
    uint32_t *lvl_one_ptr = &level_one_tables[i*1024];
    level_two_dirs[i] = (uint32_t)lvl_one_ptr | MP_PRESENT | MP_READWRITE;
  }
  *root_paging_dir_out = (size_t)level_two_dirs;
}