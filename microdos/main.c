#include <memops.h>
#include <errors.h>
#include <sys/ioports.h>
#include <volmgr.h>
#include "8259pic/8259pic.h"
#include "drivers/early_serial/serial.h"
#include "utils/gdt32.h"
#include "utils/tss32.h"
#include "mmgr/mmgr.h"
#include "mmgr/heap.h"
#include "utils/memlayout.h"
#include "utils/idt32.h"
#include "drivers/bios_disk/bios_disk.h"

void cmain() {
    //we have a little bit of assembly that ran before this function, putting the bios's
    //boot disk number in this location
    uint8_t bios_boot_disk_number = *(uint8_t *)0x500;
    uint16_t bios_cursor_location = *(uint16_t *)0x501;

    err_t e;
    early_serial_lowlevel_init();
    kputs("Hello world!\r\n");

    kprintf("BIOS gives boot disk number as 0x%x\r\n", (uint32_t)bios_boot_disk_number);

    //First things first.... let's set up our interrupt table & GDT
    setup_tss();
    setup_gdt();
    setup_interrupts(0);
    //we need to have int 0xff before we can intialise mmgr as it relies on v86 mode :(
    initialise_mmgr();

    e = relocate_kernel(KERNEL_RELOCATION_BASE);
    if(e!=ERR_NONE) {
        kprintf("ERROR Cannot relocate kernel, error 0x%x\r\n", e);
        while (1)
        {
            __asm__ volatile("nop");
        }
    }
    
    //now enter the relocated kernel
    asm __volatile__ (
        ".temp_relocate_jump:\n"
        "lea .relocate_exit, %%esi\n"
        "add %0, %%esi\n"
        "push %%esi\n"
        "ret\n"
        ".relocate_exit:\n"
        "add %0, %%esp\n"
        : 
        : "r"(KERNEL_RELOCATION_BASE)
        : "esi"
    );
    //reconfigure the interrupts to their relocated addresses
    setup_interrupts(1);
    //drop the kernel out of conventional RAM to leave it free for v86
    unmap_kernel_boot_space();
    //NOTE: due to the way that data is resolved (relative to the frame pointer/segment pointer) if you try to access
    //a string _in this function_ below here it will cause a pagefault.
    //Strings in _called_ functions are absolutely fine though.
    //TODO: investigate and adjust stack/frame accordingly after the unmap op

    //Initialise a kernel heap
    initialise_heap(0x100);
    //Configure the hardware interrupts. This also adds them to the IDT.
    setup_pic();
    sti();  //now we can enable interrupts

    //test_alloc();
    struct VolMgr *volmgr = volmgr_init(bios_boot_disk_number);
    biosdisk_scan(volmgr);

    //Idle loop
    while(1) {
        asm __volatile__("nop\nhlt");
    }
}

void test_alloc() {
    char *tempbuf = malloc(512);
    kputs("allocated tempbuf\r\n");
    kprintf("test ptr at 0x%x\r\n", tempbuf);
    free(tempbuf);
    kputs("freed tempbuf\r\n");
}

void test_exception() {
    uint32_t x = 1 / 0;
}

void __stack_chk_fail() {
    kputs("PANIC - kernel stack overflow detected\r\n");
}