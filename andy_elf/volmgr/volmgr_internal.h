#ifndef __VOLMGR_INTERNAL_H
#define __VOLMGR_INTERNAL_H

#include <volmgr.h>
#include <fs.h>

struct VolMgr_Disk {
    struct VolMgr_Disk *next;
    enum disk_type type;
    uint32_t base_addr;
    uint32_t flags;
    uint8_t volume_count;
    //More fields as needed
    uint32_t optional_signature;
    uint8_t partition_count;
    struct VolMgr_Volume *volumes;
};

struct VolMgr_Volume {
    struct VolMgr_Volume *next;
    uint32_t start_sector;
    uint32_t sector_count;
    //More fields as needed
    enum PartitionType part_type;
    void *fs_ptr; //Pointer to filesystem instance - type is determined by PartitionType
};

struct VolMgr_GlobalState {
    struct VolMgr_Disk *disk_list;
    uint8_t disk_count;
};

#endif