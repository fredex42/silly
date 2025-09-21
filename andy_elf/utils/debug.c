#include <types.h>
#include <stdio.h>
#include "debug.h"

#define STACK_TOP 0x80000

void dump_stack_32()
{
    size_t ebp, current_sp;
    asm volatile ("mov %%ebp, %0" : "=r" (ebp));
    asm volatile ("mov %%esp, %0" : "=r" (current_sp));
    kprintf("DEBUG Current Stack frame: %p\n", (void *)ebp);

    while (current_sp < STACK_TOP && ebp != 0) {
        if(current_sp == ebp) {
            //We reached the top of this stack frame
            kputs("-----------\r\nNEW FRAME\r\n-----------\r\n");
            kprintf("Frame pointer: 0x%x\r\n", (unsigned int)ebp);
            kprintf("Return addr:   0x%x\r\n", (unsigned int)(*((size_t *)(ebp + 4))));
            ebp = *((size_t *)ebp);  // Follow to next frame
        }
        kprintf("0x%x: ", (unsigned int)current_sp);
        kprintf("0x%x\r\n", (unsigned int)(*((size_t *)current_sp)));
        current_sp += 4;
    }
}