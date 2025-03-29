#include <types.h>
#include "regstate.h"
#include "v86.h"
#include "tss32.h"

struct RealModeInterrupt *realModeIVT;  //due to bootloader limitations, hard-setting the value at the top will not work

uint32_t v86_call_interrupt(uint16_t intnum, struct RegState32 regs) {
    activate_v86_tss();

    realModeIVT = (struct RealModeInterrupt*)0; //real mode IVT starts at offset 0

    struct RealModeInterrupt vector = realModeIVT[intnum];
    kprintf("DEBUG realModeIVT is at 0x%x\r\n",realModeIVT);

    kprintf("v86_call_interrupt: int 0x%x -> 0x%x:0x%x\r\n", (uint32_t)intnum, (uint32_t)vector.segment, (uint32_t)vector.offset);

    asm __volatile__(
        //before everything, ensure we have a 'fake' interrupt set up to trap us back to protected mode
        "mov $0x3f8, %%edi\n"
        "mov $0x0008, (%%edi)\n"
        "lea int_ff_trapvec, %%eax\n"
        "mov %%eax, 4(%%edi)\n"
        
        //first set up a new stack in conventional RAM
        "mov $0x7fffe, %%edi\n" //top of the new stack 
        "mov $0x10, %%eax\n"
        "mov %%ax, %%es\n"
        "std\n"                 //build downwards, i.e. decrement after stosd

        //now a stack frame on the new stack to return to the v86_call_rtn label, at our target stack
        "xor %%eax, %%eax\n"    //current stack segment
        "mov %%ss, %%ax\n"
        "stosl\n"
        "mov %%esp, %%eax\n"    //old stack pointer, plus 4 (the value we just pushed)
        "sub $0x04, %%eax\n"
        "stosl\n"
        "pushf\n"               //eflags
        "pop %%eax\n"
        "stosl\n"
        "xor %%eax, %%eax\n"
        "mov %%cs, %%ax\n"      //current code segment
        "stosl\n"
        "lea .v86_call_rtn,%%eax\n" //exit point address
        "stosl\n"

        //next set up the TSS so the interrupt-to-exit call will work
        "lea v86_tss, %%edi\n"
        //save all segment registers
        "xor %%eax, %%eax\n"
        "mov %%gs, %%ax\n"
        "mov %%eax, 0x5C(%%edi)\n"
        "mov %%fs, %%ax\n"
        "mov %%eax, 0x58(%%edi)\n"
        "mov %%ds, %%ax\n"
        "mov %%eax, 0x54(%%edi)\n"
        "mov %%ss, %%ax\n"
        "mov %%eax, 0x50(%%edi)\n"  // offset 0x50 is SS
        "mov %%eax, 8(%%edi)\n"   //offset 0x08 is SS0
        "mov %%cs, %%ax\n"
        "mov %%eax, 0x4C(%%edi)\n"
        "mov %%es, %%ax\n"
        "mov %%eax, 0x48\n"
        //now save the stack pointer
        "mov %%esp, %%eax\n"
        "mov %%eax, 4(%%edi)\n"   //offset 0x04 is ESP0

        //now set up a stack frame on the existing stack to drop into v86 mode
        "mov $0x7000, %%eax\n"  //new stack segment
        "push %%eax\n"
        "mov $0xffee, %%eax\n"  //new stack pointer (bottom of the stack we just set up)
        "push %%eax\n"
        "pushf\n"               
        "pop %%eax\n"
        "or $0x00023000, %%eax\n" //set V86 mode, IOPL=3 in eflags
        "push %%eax\n"          //eflags
        "mov %1, %%eax\n"       //new code segment
        "push %%eax\n"
        "mov %2, %%eax\n"       //entry point address
        "push %%eax\n"

        //load up the registers we were given
        "mov %3,%%eax\n"
        "mov %4,%%ebx\n"
        "mov %5,%%ecx\n"
        "mov %6,%%edx\n"
        "mov %7,%%esi\n"
        "mov %8, %%edi\n"
        "iret\n"

        ".v86_call_rtn:\n"  //note we are actually STILL in v86 mode when we get here
        "int $0xff\n"       //cause a trap
        :
        : "m"(v86_tss), "m"(vector.segment),"m"(vector.offset),"m"(regs.eax),"m"(regs.ebx),"m"(regs.ecx),"m"(regs.edx),"m"(regs.esi),"m"(regs.edi)
    );
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
        "pop %%eax\n" : :
    );
}