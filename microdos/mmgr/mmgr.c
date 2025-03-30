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
    kputs("Initialising memory manager...\r\n");
    retrieve_memory_map();
    dump_memory_map();
}
