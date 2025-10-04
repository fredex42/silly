#include <types.h>
#include <multiboot.h>

// Multiboot structures are defined inline and placed in their own
// 'multiboot' section. The linker script ensures that this section is 
// placed appropriately in the final binary.
const struct MultibootHeader {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
}  multiboot_header __attribute__((section(".multiboot"), used)) = {
    .magic = 0xE85250D6,
    .architecture = 0x00000000,
    .header_length = sizeof(struct MultibootHeader),
    .checksum = -(0xE85250D6 + 0x00000000 + sizeof(struct MultibootHeader))
};

struct MultibootTagHeader {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
};

const struct MultibootInfoRequest {
    struct MultibootTagHeader header;
    uint32_t requests[]; // Array of requested tags
} multiboot_info_request  __attribute__((section(".multiboot"), used))= {
    .header.type = 1, // Type 1 tells the bootloader what tags we want back
    .header.flags = 0,
    .header.size = sizeof(struct MultibootInfoRequest),
    //0x04 - basic memory info 0x05 bios boot device 0x06 - memory map 0x02 - boot loader name
    .requests = { MB_TAG_BASIC_MEMINFO, MB_TAG_BOOTDEV, MB_TAG_MEMORY_MAP, MB_TAG_BOOTLOADER_NAME, 0 }
};

uint32_t check_multiboot2_info(uint32_t magic, uint32_t addr) {
    kprintf("Checking for multiboot2 info at %x with magic %x\r\n", addr, magic);
    // Check the magic number to ensure we were booted by a multiboot2-compliant bootloader
    if (magic != 0x36D76289) {  //this is the magic number the bootloader passes to us
        return 0; // Not a valid multiboot2 boot
    }

    struct MultibootKernelDataHeader *header = (struct MultibootKernelDataHeader *)addr;
    kprintf("Multiboot2 info total size %d\r\n", header->total_size);

    struct MultibootTagHeader *tag = (struct MultibootTagHeader *)(addr + sizeof(struct MultibootKernelDataHeader));
    while (tag && tag->type != MB_TAG_END) {
        kprintf("next tag 0x%x size %d\r\n", tag, tag->size);
        // Process each tag as needed
        switch (tag->type) {
            case MB_TAG_BASIC_MEMINFO:
                struct MultibootTagBasicMeminfo *meminfo = (struct MultibootTagBasicMeminfo *)tag;
                kprintf("Lower memory: %d KB, Upper memory: %d KB\r\n", meminfo->mem_lower, meminfo->mem_upper);
                break;
            case MB_TAG_BOOTLOADER_NAME:
                kputs("Bootloader name tag found\r\n");
                break;
            case MB_TAG_BOOTDEV:
                kputs("Boot device tag found\r\n");
                break;
            case MB_TAG_CMDLINE:
                kputs("Command line tag found\r\n");
                break;
            case MB_TAG_MEMORY_MAP:
                kputs("Memory map tag found\r\n");
                break;
            // Add cases for other tags as needed
            default:
                kprintf("Unknown tag type %d found\r\n", tag->type);
                break;
        }
        // Move to the next tag, ensuring 8-byte alignment
        tag = (struct MultibootTagHeader *)((uint32_t)tag + ((tag->size + 7) & ~7));
    }

    return 1; // Successfully processed multiboot2 info
}