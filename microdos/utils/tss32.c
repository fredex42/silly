#include <types.h>
#include <memops.h>
#include "tss32.h"

struct TSS32 v86_tss = {};
static char v86_io_port_map[V86_IO_PORT_MAP_SIZE];
struct TSS32 ring3_tss = {};

void setup_tss()
{
    memset(&v86_tss, 0, sizeof(struct TSS32)+128);
    memset(&ring3_tss, 0, sizeof(struct TSS32));
    memset(&v86_io_port_map, 0x0, 128); //allow all i/o port operations in v86 mode
}

void activate_v86_tss()
{
    asm __volatile__(
        "mov $0x20, %%eax\n"
        "ltr %%eax"
        : : : "eax"
    );
}