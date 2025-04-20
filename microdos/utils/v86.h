#include <types.h>

struct RealModeInterrupt {
    uint16_t offset;
    uint16_t segment;
} __attribute__((packed));

uint32_t v86_call_interrupt(uint16_t intnum, struct RegState32 *regs, struct RegState32 *outregs, uint32_t *outflags);
void get_realmode_interrupt(uint16_t intnum, uint16_t *segment, uint16_t *offset);

/*
Macro description:

 //before everything, ensure we have a 'fake' interrupt set up to trap us back to protected mode 
//first set up a new stack in conventional RAM
// 0x7fffe top of the new stack 
// std sets direction flag to build downwards
//set gs,fs,ds and es to 0 in that order
// .v86_call_rtn is the out-point

    //next set up the TSS so the interrupt-to-exit call will work

    //save all segment registers


    //now set up a stack frame on the existing stack to drop into v86 mode
    //new stack segment 0x7000
    //new stack pointer 0xfffe (bottom of the stack we just set up)
    //set V86 mode, IOPL=3 in eflags
*/

/**
 * Sets up stack frames suitable for a v86 call. You must then proceed to make the call via `iret`.
 * Arguments:
 * - segment - 16-bit CS value
 * - offset  - 16-bit IP value; with `segment` this tells the processor where to start executing from
 * - setrtn  - assembler string to set the return address. An example is "lea .v86_call_rtn,%%eax\n" 
 */
#define prepare_v86_call(segment, offset, setrtn)     asm __volatile__( \
    "mov $0x3f8, %%edi\n" \
    "mov $0x0008, (%%edi)\n" \
    "lea int_ff_trapvec, %%eax\n" \
    "mov %%eax, 4(%%edi)\n" \
    "mov $0x6fffe, %%edi\n"  \
    "mov $0x10, %%eax\n" \
    "mov %%ax, %%es\n" \
    "std\n"             \
    "xor %%eax, %%eax\n"  \
    "mov %%ss, %%ax\n" \
    "stosl\n" \
    "mov %%esp, %%eax\n"  \
    "sub $0x04, %%eax\n" \
    "stosl\n" \
    "pushf\n"  \
    "pop %%eax\n" \
    "stosl\n" \
    "xor %%eax, %%eax\n" \
    "mov %%cs, %%ax\n"  \
    "stosl\n" \
    setrtn \
    "stosl\n" \
    "lea v86_tss, %%edi\n" \
    "xor %%eax, %%eax\n" \
    "mov %%gs, %%ax\n" \
    "mov %%eax, 0x5C(%%edi)\n" \
    "mov %%fs, %%ax\n" \
    "mov %%eax, 0x58(%%edi)\n" \
    "mov %%ds, %%ax\n" \
    "mov %%eax, 0x54(%%edi)\n" \
    "mov %%ss, %%ax\n" \
    "mov %%eax, 0x50(%%edi)\n" \
    "mov %%eax, 8(%%edi)\n"   \
    "mov %%cs, %%ax\n" \
    "mov %%eax, 0x4C(%%edi)\n" \
    "mov %%es, %%ax\n" \
    "mov %%eax, 0x48\n" \
    "mov %%esp, %%eax\n" \
    "mov %%eax, 4(%%edi)\n"   \
    "xor %%eax,%%eax\n" \
    "push %%eax\n" \
    "push %%eax\n" \
    "push %%eax\n" \
    "push %%eax\n" \
    "mov $0x6000, %%eax\n"  \
    "push %%eax\n" \
    "mov $0xffee, %%eax\n"  \
    "push %%eax\n" \
    "pushf\n"               \
    "pop %%eax\n" \
    "or $0x00023000, %%eax\n" \
    "push %%eax\n" \
    "mov %0, %%eax\n" \
    "push %%eax\n" \
    "mov %1, %%eax\n"  \
    "push %%eax\n" \
    : \
    : "m"(segment),"m"(offset) \
    : "eax", "edi" \
);