#include <types.h>
#include "../../utils/v86.h"
#include "../../utils/memlayout.h"
#include <volmgr.h>
#include <stdio.h>
#include <sys/mmgr.h>
#include "bios_disk.h"
#include "bios_data_area.h"

size_t biosdisk_get_parameters(uint8_t disk_number, struct BiosDriveParameters *output)
{
    uint32_t output_phys = virt_to_phys(output, NULL);

    uint16_t seg,off;
    uint32_t output_param_page = (uint32_t)output_phys & MP_ADDRESS_MASK;    //start of the memory page containg the output structure
    uint32_t output_param_off  = ((uint32_t)output & ~MP_ADDRESS_MASK);  //offset of structure in the memory page
    uint16_t err_code;  //gets the value of ax after the call, used to determine if an error happened

    kprintf("DEBUG biosdisk_get_parameters physical address of output data is 0x%x with offset 0x%x\r\n", output_param_page, output_param_off);

    k_map_page(output_param_page, 0x2000, MP_PRESENT|MP_USER|MP_READWRITE);  //map the output param page over page 2 temporarily

    output->buffer_size = sizeof(struct BiosDriveParameters);

    activate_v86_tss();
    get_realmode_interrupt(0x13, &seg, &off);

    //0x584 => V86_EXIT_TRAMPOLINE
    prepare_v86_call(seg, off, "lea 0x584, %%eax\n", KERNEL_RELOCATION_BASE);
    uint16_t *tramp_jmp = (uint16_t *)(V86_EXIT_TRAMPOLINE + 2);
    *tramp_jmp = 0xe0ff;    //ff e0 => jmp %eax

    //now make the bios call
    asm __volatile__(
        "mov $0x586, %%esi\n"   //2 bytes after the exit trampoline, this is where we return back to...
        "movb  $0xbf, (%%esi)\n"  //`mov -> edi`
        "lea .bdparam_rtn, %%eax\n"
        "add %%ebx, %%eax\n"
        "mov %%eax, 1(%%esi)\n"    //set the operand of the mov instruction to the actual return location
        "movw $0xe7ff, 5(%%esi)\n"   //jmp edi
        //now that the return is set up, lets set up the call
        "movw $0x4800, %%ax\n"
        //dl is set already by the incoming argument
        "movl %3, %%esi\n"
        "add $0x2000, %%esi\n"
        "iret\n"
        ".bdparam_rtn:\n"

        : "=a"(err_code)
        : "b"(KERNEL_RELOCATION_BASE), "d"(disk_number), "m"(output_param_off)
        : "esi", "edi"
    );
    k_unmap_page(0x2000);

    kprintf("DEBUG biosdisk_get_parameters output block should be at 0x%x\r\n", output);
    err_code = err_code >> 8;   //error code is in AH
    if(err_code != 0x0) {
        kprintf("ERROR unable to determine bios disk params for 0x%x: 0x%x\r\n", (uint32_t) disk_number, (uint32_t)err_code);
        return 0;
    }
    return output->buffer_size;
}

/**
 * Scans the BIOS to find what it thinks of the disks attached to the system
 */
void biosdisk_scan(struct VolMgr *volmgr)
{
    struct BiosDriveParameters params;
    uint8_t fixed_disk_count = *BIOS_FIXED_DISK_COUNT_PTR;
    kprintf("INFO BIOS reports %d fixed disks\r\n", (uint16_t)fixed_disk_count);

    map_v86_stackmem();
    for(int i=0; i<fixed_disk_count; i++) {
        size_t result = biosdisk_get_parameters(i+0x80, &params);
    }
    unmap_v86_stackmem();
}