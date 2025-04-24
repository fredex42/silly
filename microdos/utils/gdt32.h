#include <types.h>

struct GDTR32 {
    uint16_t size;
    uint32_t offset;
} __attribute__((packed));

struct GDTEntry32 {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags_limit_hi; //lower nybble is limit, upper nybble is flags
    uint8_t base_hi;
} __attribute__((packed));

#define GDT_ACC_PRESENT 1<<7
#define GDT_ACC_DPL(lvl) lvl<<5
#define GDT_ACC_NOTTSS 1<<4 //0 for TSS, 1 for normal segment
#define GDT_ACC_EXEC 1<<3   //0 for data segment, 1 for code segment
#define GDT_ACC_DC   1<<2   //0 for data segment growing normal, code segment can only execute from own ring. 1 for data segment growing backwards, code segment executable from equal or lower priv
#define GDT_ACC_READWR  1<<1    //0 means code segment not readable, or data segment not writable. 1 means that they work
#define GDT_ACC_ACCESSED    1<<0

#define GDT_FLAG_PAGEGRAN   1<<3
#define GDT_FLAG_32BIT      1<<2    //0 means protected-mode 16-bit seg, 1 means protected-mode 32-bit seg
#define GDT_FLAG_LONGMODE   1<<1    //1 means that this is a 64-bit code segment

void setup_gdt();
void setup_tss_gdt_entries(uint32_t base_addr);