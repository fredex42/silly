#include <types.h>
#include <x86/idt.h>
#include <stdio.h>
#include <memops.h>

struct InterruptDescriptor32 idt[INTERRUPT_MAX];

void initialize_idt() {
    memset(&idt, 0, INTERRUPT_MAX * sizeof(struct InterruptDescriptor32));
}

void print_idt_entry(struct InterruptDescriptor32* entry) {
    uint32_t offset = (entry->offset_2 << 16) | entry->offset_1;
    kprintf("IDT Entry:\n");
    kprintf("  Offset: 0x%x\n", offset);
    kprintf("  Selector: 0x%x\n", entry->selector);
    kprintf("  Type + Attributes: 0x%x\n", entry->type_attributes);
}

void create_idt_entry(uint16_t intnum, void *handler, uint16_t selector, uint8_t level) {
    if (intnum >= INTERRUPT_MAX) {
        kprintf("Error: Invalid interrupt number %d\n", intnum);
        return;
    }
    uint32_t dpl = 0;
    switch(level) {
        case 0: dpl = IDT_TYPE_DPL0; break;
        case 1: dpl = IDT_TYPE_DPL1; break;
        case 2: dpl = IDT_TYPE_DPL2; break;
        case 3: dpl = IDT_TYPE_DPL3; break;
        default:
            kprintf("Error: Invalid privilege level %d\n", level);
            return;
    }

    uint32_t handler_addr = (uint32_t)handler;
    idt[intnum].offset_1 = handler_addr & 0xFFFF;
    idt[intnum].selector = selector;
    idt[intnum].zero = 0;
    idt[intnum].type_attributes = IDT_TYPE_32BITINT | IDT_TYPE_PRESENT | dpl;
    idt[intnum].offset_2 = (handler_addr >> 16) & 0xFFFF;
}

void remove_idt_entry(uint16_t intnum) {
    if (intnum >= INTERRUPT_MAX) {
        kprintf("Error: Invalid interrupt number %d\n", intnum);
        return;
    }
    idt[intnum].offset_1 = 0;
    idt[intnum].selector = 0;
    idt[intnum].zero = 0;
    idt[intnum].type_attributes = 0;
    idt[intnum].offset_2 = 0;
}