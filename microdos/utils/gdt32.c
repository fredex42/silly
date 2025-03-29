#include <types.h>
#include "gdt32.h"
#include "tss32.h"
#include "memlayout.h"

struct GDTEntry32 *gdt;

uint32_t bswap32(uint32_t value) {
    uint32_t result;

    asm (
        "bswap %0"
        : "=r"(result) : "r"(value)
    );
    return result;
}

void configure_gdt_entry(uint8_t index, uint32_t limit, uint32_t base, uint8_t access, uint8_t flags) {
    gdt[index].limit_lo = (uint16_t)(limit & 0xFFFF);
    gdt[index].base_lo = (uint16_t)(base & 0xFFFF);
    gdt[index].base_mid = (uint8_t)( (base >> 16) & 0xFF);
    gdt[index].base_hi = (uint8_t)( (base >> 24) & 0xFF);
    gdt[index].access = access;
    gdt[index].flags_limit_hi = ((uint8_t)(limit >> 16) & 0xFF) | (flags << 4);
}

/**
 * Configure a new GDT for the kernel, so we know where we are.
 */
void setup_gdt() {
    gdt = (struct GDTEntry32 *) GDT_BASE_ADDRESS;

    memset_dw((void *)gdt, 0, 2*GDT_ENTRY_COUNT);
    //kernel CS
    configure_gdt_entry(1, 0xffffffff, 0x0, GDT_ACC_PRESENT|GDT_ACC_DPL(0)|GDT_ACC_NOTTSS|GDT_ACC_EXEC|GDT_ACC_READWR, GDT_FLAG_32BIT|GDT_FLAG_PAGEGRAN);
    //kernel DS
    configure_gdt_entry(2, 0xffffffff, 0x0, GDT_ACC_PRESENT|GDT_ACC_DPL(0)|GDT_ACC_NOTTSS|GDT_ACC_READWR, GDT_FLAG_32BIT|GDT_FLAG_PAGEGRAN);
    //display memory
    configure_gdt_entry(3, 0xBFFFF, 0xA0000, GDT_ACC_PRESENT|GDT_ACC_DPL(0)|GDT_ACC_NOTTSS|GDT_ACC_READWR, GDT_FLAG_32BIT);
    //v86 mode TSS
    configure_gdt_entry(4, sizeof(struct TSS32)+V86_IO_PORT_MAP_SIZE, (uint32_t)&v86_tss, 0x89, 0x40);
    //ring3 mode TSS
    configure_gdt_entry(5, sizeof(struct TSS32), (uint32_t)&ring3_tss, 0x89, 0x40);

    struct GDTR32 gdtr;
    gdtr.size = (4*16) - 1; //4 entries of 16 bytes each
    gdtr.offset = (uint32_t) gdt;

    asm __volatile__(
        "lgdt %0"
        :
        : "m"(gdtr)
    );

}