#include <types.h>
#include <multiboot.h>
#include <sys/mmgr.h>
#include <kernel_config.h>
#include <panic.h>
#include <volmgr.h>

//defined in acpi/rsdp.c
void load_acpi_data();

struct KernelConfig *kernel_config_ptr;

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

/**
 * Early-init for multiboot2. This function is called to determine if we have multiboot2 metadata, and if so to initialise
 * core services like the memory manager.
 */
uint32_t init_multiboot_system(uint32_t magic, uint32_t addr) {
    kernel_config_ptr = NULL;
    kprintf("Checking for multiboot2 info at %x with magic %x\r\n", addr, magic);
    // Check the magic number to ensure we were booted by a multiboot2-compliant bootloader
    if (magic != 0x36D76289) {  //this is the magic number the bootloader passes to us
        return 0; // Not a valid multiboot2 boot
    }

    struct MultibootKernelDataHeader *header = (struct MultibootKernelDataHeader *)addr;
    kprintf("Multiboot2 info total size %d\r\n", header->total_size);

    struct MultibootTagHeader *tag = (struct MultibootTagHeader *)(addr + sizeof(struct MultibootKernelDataHeader));
    while (tag && tag->type != MB_TAG_END) {
        // Process each tag as needed
        switch (tag->type) {
            case MB_TAG_BASIC_MEMINFO:
                struct MultibootTagBasicMeminfo *meminfo = (struct MultibootTagBasicMeminfo *)tag;
                kprintf("Lower memory: %d KB, Upper memory: %d KB\r\n", meminfo->mem_lower, meminfo->mem_upper);
                break;
            case MB_TAG_BOOTLOADER_NAME:
                struct MultibootTagBootloaderName *bootloader = (struct MultibootTagBootloaderName *)tag;
                kprintf("Bootloader name: %s\r\n", bootloader->name);// Null-terminated string so it's _probably_ safe :crossed-fingers:
                break;
            case MB_TAG_MEMORY_MAP:
                struct MultibootTagMemoryMap *mmap = (struct MultibootTagMemoryMap *)tag;
                kprintf("Size of memory map is 0x%x, entry size 0x%x, entry version 0x%x\r\n", mmap->size, mmap->entry_size, mmap->entry_version);
                int entries = (mmap->size - 16) / mmap->entry_size;
                kprintf("There are %d entries\r\n", entries);
                for (int i = 0; i < entries; i++) {
                    kprintf("Entry %d: Address 0x%x, Length 0x%x, Type %d\r\n", i, (uint32_t)mmap->entries[i].addr, (uint32_t)mmap->entries[i].len, mmap->entries[i].type);
                }
                initialise_mmgr(mmap->entries, entries, addr, header->total_size);
                break;
            case MB_TAG_FRAMEBUFFER:
                struct MultibootTagFramebuffer *fb = (struct MultibootTagFramebuffer *)tag;
                break;
            case MB_TAG_IMAGE_PHYS_ADDR:
                struct MultibootTagPhysAddr *img = (struct MultibootTagPhysAddr *)tag;
                kprintf("Image load address 0x%x\r\n", (uint32_t)img->image_addr);
                break;
            // Add cases for other tags as needed
            default:
                break;
        }
        // Move to the next tag, ensuring 8-byte alignment
        tag = (struct MultibootTagHeader *)((uint32_t)tag + ((tag->size + 7) & ~7));
    }

    validate_kernel_memory_allocations(0); // Check for any memory leaks at this point
    //Once we have core services like the memory manager up and running, we can do more detailed multiboot2 processing
    multiboot2_late_init(magic, addr);
    return 1; // Successfully processed multiboot2 info
}

/**
 * Late-init for multiboot2. This function is called to get multiboot2 metadata after the core services like the memory manager are up and running.
 */
uint32_t multiboot2_late_init(uint32_t magic, uint32_t addr) {
    kprintf("Checking for multiboot2 info at %x with magic %x\r\n", addr, magic);
    // Check the magic number to ensure we were booted by a multiboot2-compliant bootloader
    if (magic != 0x36D76289) {  //this is the magic number the bootloader passes to us
        return 0; // Not a valid multiboot2 boot
    }

    kernel_config_ptr = new_kernel_config();
    if(!kernel_config_ptr) {
        k_panic("FATAL Could not allocate kernel config structure\r\n");
        return 0;
    }

    struct MultibootKernelDataHeader *header = (struct MultibootKernelDataHeader *)addr;
    kprintf("Multiboot2 info total size %d\r\n", header->total_size);

    struct MultibootTagHeader *tag = (struct MultibootTagHeader *)(addr + sizeof(struct MultibootKernelDataHeader));
    while (tag && tag->type != MB_TAG_END) {
        // Process each tag as needed
        switch (tag->type) {
            case MB_TAG_BASIC_MEMINFO:
                break;
            case MB_TAG_BOOTLOADER_NAME:
                break;
            case MB_TAG_BOOTDEV:
                struct MultibootTagBiosBootDevice *bootdev = (struct MultibootTagBiosBootDevice *)tag;
                kprintf("Boot device: 0x%x:0x%x:0x%x\r\n", bootdev->biosdev, bootdev->partition, bootdev->subpartition);
                kernel_config_ptr->bios_boot_device = bootdev->biosdev;
                kernel_config_ptr->bios_partition = bootdev->partition;
                kernel_config_ptr->bios_subpartition = bootdev->subpartition;
                break;
            case MB_TAG_CMDLINE:
                struct MultibootTagCmdline *cmdline = (struct MultibootTagCmdline *)tag;
                kprintf("Kernel command line: '%s'\r\n", cmdline->cmdline);
                kernel_config_ptr->cmdline = parse_command_line(cmdline->cmdline);
                if(kernel_config_ptr->cmdline==NULL) {
                    kputs("WARNING Could not parse command line\r\n");
                }
                break;
            case MB_TAG_MEMORY_MAP:
                break;
            case MB_TAG_APM:
                struct MultibootTagAPM *apm = (struct MultibootTagAPM *)tag;
                kprintf("APM version 0x%x, cseg 0x%x, offset 0x%x, cseg_16 0x%x, dseg 0x%x, flags 0x%x, cseg_len 0x%x, cseg_16_len 0x%x, dseg_len 0x%x\r\n",
                        apm->version, apm->cseg, apm->offset, apm->cseg_16, apm->dseg, apm->flags, apm->cseg_len, apm->cseg_16_len, apm->dseg_len);
                break;
            case MB_TAG_RSDP_NEW:
                kputs("ACPI RSDP 2.0 found\r\n");
                load_acpi_data();
                break;
            case MB_TAG_RSDP_OLD:
                kputs("ACPI RSDP 1.0 found\r\n");
                load_acpi_data();
                break;
            case MB_TAG_VBE:
                struct MultibootTagVBE *vbe = (struct MultibootTagVBE *)tag;
                kprintf("VBE mode 0x%x, interface seg 0x%x, off 0x%x, len 0x%x\r\n", vbe->vbe_mode, vbe->vbe_interface_seg, vbe->vbe_interface_off, vbe->vbe_interface_len);
                break;
            case MB_TAG_FRAMEBUFFER:
                break;
            case MB_TAG_ELF_SYMS:
                break;
            case MB_TAG_IMAGE_PHYS_ADDR:
                break;
            case MB_TAG_SMBIOS:
                kputs("SMBIOS found\r\n");
                break;
            // Add cases for other tags as needed
            default:
                kprintf("Unknown tag type %d found\r\n", tag->type);
                break;
        }
        // Move to the next tag, ensuring 8-byte alignment
        tag = (struct MultibootTagHeader *)((uint32_t)tag + ((tag->size + 7) & ~7));
    }

    //Initialise the volume manager. TODO: pass in relevant commandline params (e.g., boot volume)
    volmgr_init();
    return 1; // Successfully processed multiboot2 info
}

/**
 * Returns a pointer to the kernel configuration structure, or NULL if not available.
 * The returned pointer is owned by the kernel and should not be freed nor modified.
 */
const struct KernelConfig *get_kernel_config() {
    return (const struct KernelConfig *)kernel_config_ptr;
}