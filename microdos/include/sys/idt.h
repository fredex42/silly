#include <types.h>

struct IDTR32 {
    uint16_t size;
    uint32_t offset;
} __attribute__((packed));

struct InterruptDescriptor32 {
    uint16_t offset_lo;
    uint16_t selector;  //GDT selector
    uint8_t reserved;
    uint8_t attributes;
    uint16_t offset_hi;
} __attribute__((packed));

#define IDT_LIMIT        256

#define IDT_ATTR_PRESENT 0x80
#define IDT_ATTR_DPL(lvl) lvl << 5  //use to set the privilege level from 0 to 3
#define IDT_ATTR_TASKGATE   0x05
#define IDT_ATTR_INT_16     0x06
#define IDT_ATTR_TRAP_16    0x07
#define IDT_ATTR_INT_32     0x0E
#define IDT_ATTR_TRAP_32    0x0F

//Exception handlers defined in exceptions.s
extern void IReserved();
extern void IDivZero();
extern void IDebug();
extern void INMI();
extern void IBreakPoint();
extern void IOverflow();
extern void IBoundRange();
extern void IOpcodeInval();
extern void IDevNotAvail();
extern void IInvalidTSS();
extern void ISegNotPresent();
extern void IStackSegFault();
extern void IGPF();
extern void IPageFault();
extern void IFloatingPointExcept();
extern void IAlignmentCheck();
extern void IMachineCheck();
extern void ISIMDFPExcept();
extern void IVirtExcept();
extern void ISecurityExcept();
extern void IDoubleFault();

