#include <types.h>
#include <memops.h>
#include "mmgr.h"
#include "../utils/v86.h"
#include "../utils/tss32.h"
#include "../utils/memlayout.h"
#include "../utils/regstate.h"

static struct MemoryMapEntry *memoryMap;

void retrieve_memory_map()
{
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
            "mov %0, %%ecx\n"   //offset returned from last time
            "mov $0x20, %%eax\n"    //size of a map entry
            "mul %%ecx\n"       //eax = eax * ecx
            "mov memoryMap, %%edi\n"
            "add %%eax, %%edi\n"    //edi = edi + eax

            //set up magic numbers for the call
            "xor %%eax, %%eax\n"
            "mov %%ax, %%es\n"
            "mov $0xe820, %%eax\n"
            "mov $0x534d4150, %%edx\n"
            "mov %0, %%ebx\n"
            "mov $0x20, %%ecx\n"
            "iret\n"
            ".mem_call_rtn:\n"
            "int $0xff\n" //exit v86 mode
            "mov %%ebx, %1\n"   //retrieve bx value (next offset)
            "mov %%eax, %2\n"   //retrieve ax value (error code)
            "pushf\n"
            "pop %%eax\n"
            "mov %%eax, %3\n"   //retrieve eflags value
            : "+m" (returned_bx)
            : "m" (returned_bx), "m"(returned_ax), "m"(returned_flags)
            : "eax", "ebx", "ecx", "edx", "edi"
        );
        kprintf("returned from v86, eax=0x%x, ebx=0x%x flags=0x%x\r\n", returned_ax, returned_bx, returned_flags);
    } while(!(returned_flags&0x3) || returned_bx!=0);
}

void initialise_mmgr() {
    kputs("Initialising memory manager...\r\n");
    retrieve_memory_map();
}
