#include <types.h>

struct TSS32 {
    uint16_t link;
    uint16_t reserved1;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t reserved2;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t reserved3;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t reserved4;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;    //upper 16 bits not used for segment registers of course
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint16_t ldtr;  //also 16 bits
    uint32_t reserved5;
    uint16_t iobp;   //I/O Map Base Address Field. Contains a 16-bit offset from the base of the TSS to the I/O Permission Bit Map.
    uint32_t ssp;   //shadow stack pointer
} __attribute__((packed));

extern struct TSS32 v86_tss;
extern struct TSS32 ring3_tss;

void setup_tss();
void activate_v86_tss();

#define V86_IO_PORT_MAP_SIZE 128    //allow access to the first 0x400 ports