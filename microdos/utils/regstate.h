#include <types.h>

struct RegState32 {
    uint32_t eax,ebx,ecx,edx,esi,edi;
} __attribute__((packed));