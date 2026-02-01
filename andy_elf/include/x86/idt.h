#include <types.h>
#ifndef __X86_IDT_H__
#define __X86_IDT_H__

struct idt_ptr {
        uint16_t limit;
        uint32_t base;
} __attribute__((packed));

struct InterruptDescriptor32 {
   uint16_t offset_1;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  zero;            // unused, set to 0
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_2;        // offset bits 16..31
} __attribute__((packed));


// Values for type_attributes
#define IDT_TYPE_TASKGATE   0x05 << 8
#define IDT_TYPE_16BITINT   0x06 << 8
#define IDT_TYPE_16BITTRAP  0x07 << 8
#define IDT_TYPE_32BITINT   0x0E << 8
#define IDT_TYPE_32BITTRAP  0x0F << 8

// Values for DPL (Descriptor Privilege Level) - minimum privilege level required to access this interrupt/trap from software
#define IDT_TYPE_DPL0     0x00 << 13
#define IDT_TYPE_DPL1     0x01 << 13
#define IDT_TYPE_DPL2     0x02 << 13
#define IDT_TYPE_DPL3     0x03 << 13

#define IDT_TYPE_PRESENT   0x01 << 15   //must be set for entry to be valid

#define INTERRUPT_MAX 256

//Function prototypes
void create_idt_entry(uint16_t intnum, void *handler, uint16_t selector, uint8_t level);
void remove_idt_entry(uint16_t intnum);
void print_idt_entry(struct InterruptDescriptor32* entry);
void initialize_idt();

#endif // __X86_IDT_H__