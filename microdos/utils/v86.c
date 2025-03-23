#include <types.h>
#include "regstate.h"
#include "v86.h"

struct RealModeInterrupt *realModeIVT;  //due to bootloader limitations, hard-setting the value at the top will not work

uint32_t v86_call_interrupt(uint16_t intnum, struct RegState32 regs) {
    realModeIVT = (struct RealModeInterrupt*)0; //real mode IVT starts at offset 0

    struct RealModeInterrupt vector = realModeIVT[intnum];
    kprintf("DEBUG realModeIVT is at 0x%x\r\n",realModeIVT);

    kprintf("v86_call_interrupt: int 0x%x -> 0x%x:0x%x\r\n", (uint32_t)intnum, (uint32_t)vector.segment, (uint32_t)vector.offset);

    asm __volatile__(
        //first set up a stack frame to return to the v86_call_rtn label
        "pushf\n"
        "xor %%eax, %%eax\n"
        "mov %%cs, %%ax\n"
        "push %%eax\n"
        "mov .v86_call_rtn,%%eax\n"
        "push %%eax\n"

        //now set up a stack frame to drop into v86 mode
        "pushf\n"
        "pop %%eax\n"
        "or $0x00023000, %%eax\n" //set V86 mode, IOPL=3 in eflags
        "push %%eax\n"
        "mov %0, %%eax\n"
        "push %%eax\n"
        "mov %1, %%eax\n"
        "push %%eax\n"

        //load up the registers we were given
        "mov %2,%%eax\n"
        "mov %3,%%ebx\n"
        "mov %4,%%ecx\n"
        "mov %5,%%edx\n"
        "mov %6,%%edi\n"
        "mov %7, %%edi\n"
        "iret\n"
        ".v86_call_rtn:\n"

        :
        : "m"(vector.segment),"m"(vector.offset),"m"(regs.eax),"m"(regs.ebx),"m"(regs.ecx),"m"(regs.edx),"m"(regs.esi),"m"(regs.edi)
    );
}