#include <types.h>
#include "../../utils/v86.h"
#include <volmgr.h>
#include <stdio.h>
#include <sys/mmgr.h>
#include "bios_disk.h"
#include "bios_data_area.h"

size_t biosdisk_get_parameters(uint8_t disk_number, struct BiosDriveParameters *output)
{
    uint16_t seg,off;
    uint32_t output_param_page = (uint32_t)output & MP_ADDRESS_MASK;    //start of the memory page containg the output structure
    uint32_t output_param_off  = (uint32_t)output - output_param_page;  //offset of structure in the memory page
    uint16_t err_code;  //gets the value of ax after the call, used to determine if an error happened

    k_map_page(output_param_page, 0x2000, MP_PRESENT|MP_USER|MP_READWRITE);  //map the output param page over page 2 temporarily

    output->buffer_size = sizeof(struct BiosDriveParameters);

    activate_v86_tss();
    get_realmode_interrupt(0x13, &seg, &off);

    prepare_v86_call(seg, off, "lea .bdparam_rtn, %%eax\n");

    //now make the bios call
    asm __volatile__(
        "mov $0x0048, %%ax\n"
        //dl is set already by the incoming argument
        "mov $0x200, %%bx\n"
        "mov %%ds, %%bx\n"
        "xor %%si, %%si\n"
        "iret\n"
        ".bdparam_rtn:\n"
        : "=a"(err_code)
        : "d"(disk_number) 
        : "ebx", "esi"
    );
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
    kprintf("BIOS reports %d fixed disks\r\n", (uint16_t)fixed_disk_count);

    for(int i=0; i<fixed_disk_count; i++) {
        size_t result = biosdisk_get_parameters(i+0x80, &params);
    }
}