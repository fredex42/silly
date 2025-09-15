#include <types.h>

#ifndef __VOLMGR_MBR_H
#define __VOLMGR_MBR_H

/*
Defines the data layout of an MBR partition header
*/
struct MBRPartition {
    uint8_t boot_indicator; //0x00 - not bootable, 0x80 - bootable
    uint8_t start_head;
    uint16_t start_sector_cylinder; //unpack this field with the EXTRACT_CYLINDER and EXTRACT_SECTOR macros below
    uint8_t system_id;  //see the list of partition types in fstypes.h
    uint8_t end_head;
    uint16_t end_sector_cylinder;
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed));

//Sector value is the lower 6 bits, corresponding to a mask of 0x3F
#define EXTRACT_SECTOR(sector_cylinder_value) (uint8_t)(sector_cylinder_value & 0x3F)
//Cylinder value is the upper 10 bits - so shift right by 6 bits to get it
#define EXTRACT_CYLINDER(sector_cylinder_value) (uint16_t)(sector_cylinder_value >> 6)

#define MBR_OFFSET_OPTIONAL_DISKID  0x1B8   //an optional 4 byte disk identifier


//Function prototypes
err_t volmgr_init_mbr_partition(struct MBRPartition *header, struct DiskDescriptor *disk, struct VolumeDescriptor **descriptor, size_t index);

#endif