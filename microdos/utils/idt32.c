#include <types.h>
#include <sys/idt.h>
#include "memlayout.h"
#include "idt32.h"
#include "../8259pic/interrupts.h"

void configure_interrupt(struct InterruptDescriptor32 *idt, uint8_t idx, void *ptr, uint8_t type, uint8_t with_relocation) {
    idt[idx].offset_lo = (uint16_t) ((uint32_t)ptr & 0xFFFF);
    idt[idx].selector = 0x08;   //kernel CS
    idt[idx].attributes = type | IDT_ATTR_PRESENT | IDT_ATTR_DPL(0);
    uint32_t upper_bits = (uint32_t)ptr >> 16;
    if(upper_bits==0 && with_relocation) {
        upper_bits = KERNEL_RELOCATION_BASE >> 16;
    }
    idt[idx].offset_hi = (uint16_t) upper_bits;
}

void setup_interrupts(uint8_t with_relocation) {
    struct InterruptDescriptor32 *idt = (struct InterruptDescriptor32 *)IDT_BASE_ADDRESS;

    configure_interrupt(idt,0, IDivZero, IDT_ATTR_TRAP_32, with_relocation);
    configure_interrupt(idt,1, IDebug, IDT_ATTR_TRAP_32, with_relocation);
    configure_interrupt(idt,2, INMI, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,3, IBreakPoint, IDT_ATTR_TRAP_32, with_relocation);
    configure_interrupt(idt,4, IOverflow, IDT_ATTR_TRAP_32, with_relocation);
    configure_interrupt(idt,5, IBoundRange, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,6, IOpcodeInval, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,7, IDevNotAvail, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,8, IDoubleFault, IDT_ATTR_TRAP_32, with_relocation);
    configure_interrupt(idt,9, IReserved, IDT_ATTR_INT_32, with_relocation); //deprecated coprocessor segment overrun
    configure_interrupt(idt,0x0a, IBoundRange, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x0b, ISegNotPresent, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x0c, IStackSegFault, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x0d, IGPF, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x0e, IPageFault, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x0f, IReserved, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x10, IFloatingPointExcept, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x11, IAlignmentCheck, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x12, IMachineCheck, IDT_ATTR_TRAP_32, with_relocation);
    configure_interrupt(idt,0x13, ISIMDFPExcept, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x14, IVirtExcept, IDT_ATTR_INT_32, with_relocation);
    for(register int i=0x15; i<=0x1D; i++) {
        configure_interrupt(idt,i, IReserved, IDT_ATTR_INT_32, with_relocation);
    }
    configure_interrupt(idt,0x1e, ISecurityExcept, IDT_ATTR_INT_32, with_relocation);
    configure_interrupt(idt,0x1f, IReserved, IDT_ATTR_INT_32, with_relocation);

    //set up special return-from-v86 trap
    idt[0xff].offset_lo = (uint16_t) ((uint32_t)int_ff_trapvec & 0xFFFF);
    idt[0xff].selector = 0x08;   //kernel CS
    idt[0xff].attributes = IDT_ATTR_TRAP_32 | IDT_ATTR_PRESENT | IDT_ATTR_DPL(3);
    if(with_relocation) {
        idt[0xff].offset_hi = (uint16_t) (KERNEL_RELOCATION_BASE >> 16);
    } else {
        idt[0xff].offset_hi = 0;
    }
    struct IDTR32 *idtr = (struct IDTR32 *)IDTR_BASE_ADDRESS;

    idtr->size = IDT_LIMIT*2*sizeof(uint32_t);
    idtr->offset = IDT_BASE_ADDRESS;
    asm __volatile__(
        ".temp_configure_idtr:\n"
        "lidt (%0)\n" : : "r"(IDTR_BASE_ADDRESS): "esi"
    );
}

/*
Sets up interrupt handlers for the legacy PIC (ISA style IRQs)
*/
void configure_pic_interrupts(uint8_t starting_vector)
{
    struct InterruptDescriptor32 *idt = (struct InterruptDescriptor32 *)IDT_BASE_ADDRESS;

	configure_interrupt(idt, starting_vector, ITimer, IDT_ATTR_INT_32, 1);  //IRQ0 - timer
    configure_interrupt(idt, starting_vector+1, IDummy, IDT_ATTR_INT_32, 1); //IRQ1 - keyboard
    configure_interrupt(idt, starting_vector+2, IDummy, IDT_ATTR_INT_32, 1);    //IRQ2 - cascade, we don't see it
    configure_interrupt(idt, starting_vector+3, ISerial2, IDT_ATTR_INT_32, 1);  //IRQ3 - serial port 2
    configure_interrupt(idt, starting_vector+4, ISerial1, IDT_ATTR_INT_32, 1);  //IRQ4 - serial port 1
    configure_interrupt(idt, starting_vector+5, IParallel2, IDT_ATTR_INT_32, 1);  //IRQ5 - parallel port 1
    configure_interrupt(idt, starting_vector+6, IFloppy1, IDT_ATTR_INT_32, 1);  //IRQ6 - FDD1
    configure_interrupt(idt, starting_vector+7, ISpurious, IDT_ATTR_INT_32, 1);  //IRQ7 - spurious irq
    configure_interrupt(idt, starting_vector+8, IDummy, IDT_ATTR_INT_32, 1);  //IRQ8 - CMOS clock
    configure_interrupt(idt, starting_vector+9, IRQ9, IDT_ATTR_INT_32, 1);  //IRQ9 - generic
    configure_interrupt(idt, starting_vector+10, IRQ10, IDT_ATTR_INT_32, 1);  //IRQ10 - generic
    configure_interrupt(idt, starting_vector+11, IRQ11, IDT_ATTR_INT_32, 1);  //IRQ11 - generic
    configure_interrupt(idt, starting_vector+12, IMouse, IDT_ATTR_INT_32, 1);  //IRQ12 - mouse
    configure_interrupt(idt, starting_vector+13, IFPU, IDT_ATTR_INT_32, 1);  //IRQ13 - floating-point unit
    configure_interrupt(idt, starting_vector+14, IPrimaryATA, IDT_ATTR_INT_32, 1);  //IRQ14 - IDE1
    configure_interrupt(idt, starting_vector+15, ISecondaryATA, IDT_ATTR_INT_32, 1);  //IRQ15 - IDE2
}