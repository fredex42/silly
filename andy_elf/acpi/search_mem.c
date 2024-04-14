#include "search_mem.h"
#include <sys/mmgr.h>
#include <panic.h>

uint8_t *map_bios_area()
{
    /*   We want to start the search at 0xE0000 and run until 0xFFFFF (in physical RAM).
    So, we need to map in 0x20 pages from physical address 0xE0000 to access it.
    */
    uint32_t phys_ptrs[0x20];
    for (register uint32_t i=0; i<0x20; i+= 1) {
        phys_ptrs[i] = 0xE0000 + (i * 0x1000);
    }
    kprintf("Mapping BIOS area into kernel...\r\n");
    uint8_t *buffer = (uint8_t *)vm_map_next_unallocated_pages(NULL, MP_PRESENT, (void **)&phys_ptrs, 0x20);
    if(!buffer) {
        k_panic("Cannot map memory to scan for ACPI");
    }
    return buffer;
}

void unmap_bios_area(uint8_t *buffer)
{
    //Unmap the pages
    kprintf("Unmapping BIOS area..\r\n");
    for (register uint32_t i=0; i<0x20; i+=1) {
        k_unmap_page_ptr(NULL, (void *)(buffer + 0x1000*i));
    }
}

/*
;Purpose: attempt to find the ACPI RSDP structure in the main BIOS area
;Expects: Nothing
;Returns: A pointer to the RSDP structure or NULL if not found.
*/
const struct RSDPDescriptor* scan_memory_for_acpi(uint8_t *buffer)
{
    kprintf("DEBUG scan_memory_for_acpi mapped buffer at 0x%x\r\n", buffer);
for (register uint32_t i=0; i<0x20000; i+=1) {
    if(
        buffer[i]   == 'R' &&
        buffer[i+1] == 'S' &&
        buffer[i+2] == 'D' &&
        buffer[i+3] == ' ' &&
        buffer[i+4] == 'P' &&
        buffer[i+5] == 'T' &&
        buffer[i+6] == 'R' &&
        buffer[i+7] == ' '
    ) {
        //We found it!
        return (const struct RSDPDescriptor*)&buffer[i];
    }
}

return NULL;
}