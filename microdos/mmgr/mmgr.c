#include <types.h>
#include <memops.h>
#include "mmgr.h"
#include "../utils/v86.h"
#include "../utils/memlayout.h"
#include "../utils/regstate.h"

static struct MemoryMapEntry *memoryMap;

void retrieve_memory_map()
{
    memoryMap = (struct MemoryMapEntry *) MEMORY_MAP_STATICPTR;
    memset(memoryMap, 0, MEMORY_MAP_LIMIT);

    //https://www.ctyme.com/intr/rb-1741.htm
    struct RegState32 v86regs = {
        eax: 0x000E820,
        edx: 0x534D4150,
        ebx: 0x0,
        ecx: 0x20,
        edi: MEMORY_MAP_STATICPTR,  //hmm this should work but only because es happens to be 0x0 when we go to v86
    };
    struct RegState32 outregs;
    uint32_t outflags = 0;

    v86_call_interrupt(0x15, &v86regs, &outregs, &outflags);

    kprintf("returned from v86_call_interrupt, eax=0x%x, flags=0x%x\r\n", outregs.eax, outflags);
}

void initialise_mmgr() {
    kputs("Initialising memory manager...\r\n");
    retrieve_memory_map();
}
