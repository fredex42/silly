#include <sys/idt.h>
#include <memops.h>

void _start() {
    kputs("Hello world!\r\n");

    //First things first.... let's set up our interrupt table
    setup_interrupts();

    v86_call_interrupt(0, NULL);
    
    while(1) {
        __asm__ volatile("nop");
    }
}

static struct IDTR32 idtr;
static struct InterruptDescriptor32 idt[IDT_LIMIT];

void configure_interrupt(uint8_t idx, void *ptr, uint8_t type) {
    idt[idx].offset_lo = (uint16_t) ((uint32_t)ptr & 0xFFFF);
    idt[idx].selector = 0x08;   //kernel CS
    idt[idx].attributes = type | IDT_ATTR_PRESENT | IDT_ATTR_DPL(0);
    idt[idx].offset_hi = (uint16_t) ((uint32_t)ptr >> 16);
}

void setup_interrupts() {
    memset_dw(&idt, 0, IDT_LIMIT*2);
    configure_interrupt(0, IDivZero, IDT_ATTR_TRAP_32);
    configure_interrupt(1, IDebug, IDT_ATTR_TRAP_32);
    configure_interrupt(2, INMI, IDT_ATTR_INT_32);
    configure_interrupt(3, IBreakPoint, IDT_ATTR_TRAP_32);
    configure_interrupt(4, IOverflow, IDT_ATTR_TRAP_32);
    configure_interrupt(5, IBoundRange, IDT_ATTR_INT_32);
    configure_interrupt(6, IOpcodeInval, IDT_ATTR_INT_32);
    configure_interrupt(7, IDevNotAvail, IDT_ATTR_INT_32);
    configure_interrupt(8, IDoubleFault, IDT_ATTR_TRAP_32);
    configure_interrupt(9, IReserved, IDT_ATTR_INT_32); //deprecated coprocessor segment overrun
    configure_interrupt(0x0a, IBoundRange, IDT_ATTR_INT_32);
    configure_interrupt(0x0b, ISegNotPresent, IDT_ATTR_INT_32);
    configure_interrupt(0x0c, IStackSegFault, IDT_ATTR_INT_32);
    configure_interrupt(0x0d, IGPF, IDT_ATTR_INT_32);
    configure_interrupt(0x0e, IPageFault, IDT_ATTR_INT_32);
    configure_interrupt(0x0f, IReserved, IDT_ATTR_INT_32);
    configure_interrupt(0x10, IFloatingPointExcept, IDT_ATTR_INT_32);
    configure_interrupt(0x11, IAlignmentCheck, IDT_ATTR_INT_32);
    configure_interrupt(0x12, IMachineCheck, IDT_ATTR_TRAP_32);
    configure_interrupt(0x13, ISIMDFPExcept, IDT_ATTR_INT_32);
    configure_interrupt(0x14, IVirtExcept, IDT_ATTR_INT_32);
    for(register int i=0x15; i<=0x1D; i++) {
        configure_interrupt(i, IReserved, IDT_ATTR_INT_32);
    }
    configure_interrupt(0x1e, ISecurityExcept, IDT_ATTR_INT_32);
    configure_interrupt(0x1f, IReserved, IDT_ATTR_INT_32);

    idtr.size = IDT_LIMIT*2*sizeof(uint32_t);
    idtr.offset = (uint32_t) &idt;
    asm __volatile__(
        "lidt %0" : : "m"(idtr)
    );
}
