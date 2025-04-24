#include <types.h>
#include <errors.h>
#include <sys/mmgr.h>
#include "regstate.h"
#include "v86.h"
#include "tss32.h"
#include "idt32.h"  //for prototype of int_ff_trapvec

/**
 * Retrieve the segment/offset for a real mode interrupt from the 16-bit IVT
 */
void get_realmode_interrupt(uint16_t intnum, uint16_t *segment, uint16_t *offset)
{
    struct RealModeInterrupt *realModeIVT = (struct RealModeInterrupt*)0; //real mode IVT starts at offset 0
    struct RealModeInterrupt vector = realModeIVT[intnum];
    kprintf("get_realmode_interrupt: int 0x%x -> 0x%x:0x%x\r\n", (uint32_t)intnum, (uint32_t)vector.segment, (uint32_t)vector.offset);
    *segment = vector.segment;
    *offset = vector.offset;
}

/**
 * It is necessary to call this function before making a v86 call once the kernel is remapped.
 * It ensures that free memory is placed into pages 0x7f and 0x6f for the v86 stacks use.
 * 
 * You must call unmap_v86_stackmem when done to free the pages.
 */
err_t map_v86_stackmem()
{
    err_t e;
    void *mem = vm_alloc_pages(NULL, 2, MP_USER|MP_READWRITE|MP_PRESENT);
    if(mem==NULL) {
        kputs("ERROR Unable to allocate 2 pages for v86 stack\r\n");
        return ERR_NOMEM;
    }

    void *mem2 = (void *)((size_t)mem + PAGE_SIZE);
    //vm_alloc_pages should always give us identity-mapped pointers, so we can use as a physptr.
    //V86 tasks always run in ring3 so we need to have MP_USER
    e=k_map_page(mem, 0x6f000, MP_USER|MP_READWRITE|MP_PRESENT);
    if(e!=ERR_NONE) {
        kputs("ERROR map_v86_stackmem unable to map first page\r\n");
        return e;
    }

    k_map_page(mem2, 0x70000, MP_USER|MP_READWRITE|MP_PRESENT);
    if(e!=ERR_NONE) {
        kputs("ERROR map_v86_stackmem unable to map second page\r\n");
        return e;
    }

    //ensure that the exit trampoline is set up
    uint16_t *tram = (uint16_t *)V86_EXIT_TRAMPOLINE;
    *tram = 0xffcd; //0xCD 0xFF - int 256. Remember byte reversal.

    return ERR_NONE;
}


void unmap_v86_stackmem()
{
    void *p1_phys = virt_to_phys((void *)0x6f000, NULL);
    if(p1_phys) {
        kprintf("INFO unmap_v86_stackmem physical ptr for page 1 is 0x%x\r\n", p1_phys);
        k_unmap_page(0x6f000);
        vm_deallocate_physical_pages(NULL, p1_phys, 1);
    }
    void *p2_phys = virt_to_phys((void *)0x7f000, NULL);
    if(p2_phys) {
        kprintf("INFO unmap_v86_stackmem physical ptr for page 1 is 0x%x\r\n", p2_phys);
        k_unmap_page(0x7f000);
        vm_deallocate_physical_pages(NULL, p2_phys, 1);
    }
}

uint32_t v86_call_interrupt(uint16_t intnum, struct RegState32 *regs, struct RegState32 *outregs, uint32_t *outflags) {
    activate_v86_tss();

    struct RealModeInterrupt *realModeIVT = (struct RealModeInterrupt*)0; //real mode IVT starts at offset 0

    struct RealModeInterrupt vector = realModeIVT[intnum];
    kprintf("DEBUG realModeIVT is at 0x%x\r\n",realModeIVT);

    kprintf("v86_call_interrupt: int 0x%x -> 0x%x:0x%x\r\n", (uint32_t)intnum, (uint32_t)vector.segment, (uint32_t)vector.offset);

    uint32_t tempinax = regs->eax;
    kprintf("tempinax = 0x%x\r\n", tempinax);
    uint32_t tempinbx = regs->ebx;
    uint32_t tempincx = regs->ecx;
    uint32_t tempindx = regs->edx;
    kprintf("tempindx = 0x%x\r\n", tempindx);
    uint32_t tempinsi = regs->esi;
    uint32_t tempindi = regs->edi;

    uint32_t tempax,tempbx,tempcx,tempdx,tempsi,tempdi,tempflags;

    prepare_v86_call(vector.segment, vector.offset, "lea .v86_call_rtn,%%eax\n", 0);

    asm __volatile__(
        //load up the registers we were given
        "mov %0,%%eax\n"
        "mov %1,%%ebx\n"
        "mov %2,%%ecx\n"
        "mov %3,%%edx\n"
        "mov %4,%%esi\n"
        "mov %5, %%edi\n"
        "iret\n"

        ".v86_call_rtn:\n"  //note we are actually STILL in v86 mode when we get here
        "int $0xff\n"       //cause a trap
        "mov %%eax, %0\n"                  //the trap returns here and we have left v86 mode, but registers should still be the same
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        "mov %%esi, %4\n"
        "mov %%edi, %5\n"
        "pushf\n"
        "pop %%eax\n"
        "mov %%eax, %6\n"
        : "=m"(tempax),"=m"(tempbx),"=m"(tempcx),"=m"(tempdx),"=m"(tempsi), "=m"(tempdi), "=m"(tempflags)
        : "m"(tempinax),"m"(tempinbx),"m"(tempincx),"m"(tempindx),"m"(tempinsi),"m"(tempindi)
        : "eax","ebx","ecx","edx","esi","edi"
    );
    outregs->eax = tempax;
    outregs->ebx = tempbx;
    outregs->ecx = tempcx;
    outregs->edx = tempdx;
    outregs->edi = tempdi;
    outregs->esi = tempsi;
    *outflags = tempflags;
}

void int_ff_trapvec() {
    //reset the segment registers to standard kernel values
    asm __volatile__ (
        "push %%eax\n"
        "mov $0x18, %%eax\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%es\n"
        "mov $0x10, %%eax\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ds\n"
        //restore the IO permission level
        "pushf\n"
        "pop %%eax\n"
        "and $0xFFFFCFFF, %%eax\n"  //clear IOPL bits
        "push %%eax\n"
        "popf\n"
        "pop %%eax\n" : :
    );
    //When this function returns, it does so on the stack that `prepare_v86_call` set up and will therefore go back
    //to the operation after the int 0xff call but in full protected mode.
}