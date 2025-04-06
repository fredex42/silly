#include <types.h>
#include <stdio.h>
#include <panic.h>

void k_panic(char *msg) {
    kputs(msg);
    while(1) {
        asm __volatile__(
            "nop\n"
            "hlt\n"
        );
    }
}