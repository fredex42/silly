#include <types.h>
#include <memops.h>
#include "tss32.h"

struct TSS32 v86_tss = {};
struct TSS32 ring3_tss = {};

void setup_tss()
{
    memset(&v86_tss, 0, sizeof(struct TSS32));
    memset(&ring3_tss, 0, sizeof(struct TSS32));
}

void activate_v86_tss()
{
    asm __volatile__(
        "mov $0x20, %%eax\n"
        "ltr %%eax"
        : : : "eax"
    );
}