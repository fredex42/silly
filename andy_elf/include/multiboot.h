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
#define MB_TAG_ELF_SYMS 9   //kernel elf symbol table
#define MB_TAG_IMAGE_PHYS_ADDR 21   //physical address of the image loaded

/*
This defines the start of the data sent to the kernel by grub
*/
struct MultibootKernelDataHeader {
    uint32_t total_size;
    uint32_t reserved;
}  __attribute__((packed));

// See https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Boot-information-format

struct MultibootTagBasicMeminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));

struct MultibootTagBootloaderName {
    uint32_t type;
    uint32_t size;
    char name[]; // Null-terminated string
} __attribute__((packed));


struct MultibootTagBiosBootDevice {
    uint32_t type;
    uint32_t size;
    /*
    ‘partition’ specifies the top-level partition number, ‘sub_partition’ specifies a sub-partition in the top-level partition, etc. Partition numbers always start from zero. Unused partition bytes must be set to 0xFFFFFFFF. For example, if the disk is partitioned using a simple one-level DOS partitioning scheme, then ‘partition’ contains the DOS partition number, and ‘sub_partition’ if 0xFFFFFF. As another example, if a disk is partitioned first into DOS partitions, and then one of those DOS partitions is subdivided into several BSD partitions using BSD’s disklabel strategy, then ‘partition’ contains the DOS partition number and ‘sub_partition’ contains the BSD sub-partition within that DOS partition.

DOS extended partitions are indicated as partition numbers starting from 4 and increasing, rather than as nested sub-partitions, even though the underlying disk layout of extended partitions is hierarchical in nature. For example, if the boot loader boots from the second extended partition on a disk partitioned in conventional DOS style, then ‘partition’ will be 5, and ‘sub_partiton’ will be 0xFFFFFFFF. 
*/
    uint32_t biosdev;
    uint32_t partition;
    uint32_t subpartition;
} __attribute__((packed));

struct MultibootTagCmdline {
    uint32_t type;
    uint32_t size;
    char cmdline[]; // Null-terminated string
} __attribute__((packed));

#define MBT_MEM_TYPE_AVAILABLE 1
#define MBT_MEM_TYPE_RESERVED 2
#define MBT_MEM_TYPE_ACPI_RECLAIMABLE 3
#define MBT_MEM_TYPE_NVS 4  //preserve on hibernation
#define MBT_MEM_TYPE_BADRAM 5

struct MultibootTagMemoryMap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct {
        uint64_t addr;
        uint64_t len;
        uint32_t type;
        uint32_t reserved;
    } entries[]; // Array of memory map entries
} __attribute__((packed));

struct MultibootTagAPM {
    uint32_t type;
    uint32_t size;
    uint16_t version;
    uint16_t cseg;
    uint32_t offset;
    uint16_t cseg_16;
    uint16_t dseg;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg_16_len;
    uint16_t dseg_len;
} __attribute__((packed));

struct MultibootTagVBE {
    uint32_t type;
    uint32_t size;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed));

struct MultibootTagFramebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    union {
        struct {
            uint8_t red_field_position;
            uint8_t red_mask_size;
            uint8_t green_field_position;
            uint8_t green_mask_size;
            uint8_t blue_field_position;
            uint8_t blue_mask_size;
        };
        struct {
            uint32_t palette_num_colors;
            struct {
                uint8_t red;
                uint8_t green;
                uint8_t blue;
            } palette_colors[];
        };
    };
} __attribute__((packed));

struct MultibootTagSMBIOS {
    uint32_t type;
    uint32_t size;
    uint8_t major;
    uint8_t minor;
    uint8_t reserved[6];
    uint8_t smbios_table[];
} __attribute__((packed));

struct MultibootTagRSDPv1 {
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[];
} __attribute__((packed));

struct MultibootTagRSDPv2 {
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[];
} __attribute__((packed));

struct MultibootTagPhysAddr {
    uint32_t type;
    uint32_t size;
    uint32_t image_addr;
} __attribute__((packed));

#endif