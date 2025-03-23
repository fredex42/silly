#include <types.h>

struct RealModeInterrupt {
    uint16_t offset;
    uint16_t segment;
} __attribute__((packed));

uint32_t v86_call_interrupt(uint16_t intnum, struct RegState32 regs);