#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#define MB_TAG_BASIC_MEMINFO 4
#define MB_TAG_BOOTLOADER_NAME 2
#define MB_TAG_END 0
#define MB_TAG_BOOTDEV 5
#define MB_TAG_CMDLINE 1
#define MB_TAG_MEMORY_MAP 6
#define MB_TAG_VBE 7
#define MB_TAG_APM 10
#define MB_TAG_FRAMEBUFFER 8
#define MB_TAG_SMBIOS 13
#define MB_TAG_RSDP_OLD 14
#define MB_TAG_RSDP_NEW 15


/*
This defines the start of the data sent to the kernel by grub
*/
struct MultibootKernelDataHeader {
    uint32_t total_size;
    uint32_t reserved;
}  __attribute__((packed));

struct MultibootTagBasicMeminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));
#endif