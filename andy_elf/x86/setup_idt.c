#include <types.h>
#include <x86/idt.h>
#include <x86/segmentation.h>
#include "setup_idt.h"
#include <stdio.h>

extern struct InterruptDescriptor32 idt[INTERRUPT_MAX];
static struct idt_ptr idt_descriptor;

void setup_idt() {
    create_idt_entry(0, IDivZero, IDT_SELECTOR_CODE, 0);          // Divide by Zero
    create_idt_entry(1, IDebug, IDT_SELECTOR_CODE, 0);
    create_idt_entry(2, INMI, IDT_SELECTOR_CODE, 0);              // Non-Maskable Interrupt
    create_idt_entry(3, IBreakPoint, IDT_SELECTOR_CODE, 0);        // Breakpoint
    create_idt_entry(4, IOverflow, IDT_SELECTOR_CODE, 0);          // Overflow
    create_idt_entry(5, IBoundRange, IDT_SELECTOR_CODE, 0);        // BOUND Range Exceeded
    create_idt_entry(6, IOpcodeInval, IDT_SELECTOR_CODE, 0);        // Invalid Opcode
    create_idt_entry(7, IDevNotAvail, IDT_SELECTOR_CODE, 0);        // Device Not Available
    create_idt_entry(8, IDoubleFault, IDT_SELECTOR_CODE, 0);        // Double Fault
    create_idt_entry(9, IReserved, IDT_SELECTOR_CODE, 0);          // Reserved
    create_idt_entry(10, IInvalidTSS, IDT_SELECTOR_CODE, 0);       // Invalid TSS
    create_idt_entry(11, ISegNotPresent, IDT_SELECTOR_CODE, 0);      // Segment Not Present
    create_idt_entry(12, IStackSegFault, IDT_SELECTOR_CODE, 0);     // Stack-Segment Fault
    create_idt_entry(13, IGPF, IDT_SELECTOR_CODE, 0);              // General Protection Fault
    create_idt_entry(14, IPageFault, IDT_SELECTOR_CODE, 0);           // Page Fault
    create_idt_entry(15, IReserved, IDT_SELECTOR_CODE, 0);          // Reserved
    create_idt_entry(16, IFloatingPointExcept, IDT_SELECTOR_CODE, 0); // x87 FPU Floating-Point Error
    create_idt_entry(17, IAlignmentCheck, IDT_SELECTOR_CODE, 0);    // Alignment Check
    create_idt_entry(18, IMachineCheck, IDT_SELECTOR_CODE, 0);      // Machine Check
    create_idt_entry(19, ISIMDFPExcept, IDT_SELECTOR_CODE, 0);     // SIMD Floating-Point Exception
    create_idt_entry(20, IVirtExcept, IDT_SELECTOR_CODE, 0);        // Virtualization Exception
    // Next 8 interrupts (21-28) are reserved
    create_idt_entry(0x1E, ISecurityExcept, IDT_SELECTOR_CODE, 0);   // Security Exception
    // External PIC interrupts are set up by the PIC initialization code

    idt_descriptor.limit = (sizeof(struct InterruptDescriptor32) * INTERRUPT_MAX) - 1;
    idt_descriptor.base = (uint32_t)&idt;

    //Now tell the CPU to use it
    asm volatile("lidt (%0)" : : "m"(idt_descriptor) : "memory");
}