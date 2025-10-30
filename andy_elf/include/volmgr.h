/* This file defines the public interface of the volume manager service */
#ifndef __VOLMGR_H
#define __VOLMGR_H
#include <types.h>

enum disk_type {
    DISK_TYPE_UNKNOWN = 0,
    DISK_TYPE_ISA_IDE,
    DISK_TYPE_PCI_IDE
};

//Bitmasks for flags
#define DF_REMOVABLE    1<<0
#define DF_BUSMASTER    1<<1
#define DF_READONLY     1<<2
#define DF_LBA_SUPPORT  1<<3
#define DF_IDE_MASTER   1<<30
#define DF_IDE_SLAVE    1<<31

void volmgr_init();
//When a disk is added, the partitions are automatically enumerated and added as volumes
uint8_t volmgr_add_disk(enum disk_type type, uint32_t base_addr, uint32_t flags);
uint8_t volmgr_get_disk_count();
void *volmgr_disk_for_id(uint8_t id);

//Called internally to add a new disk
uint8_t volmgr_add_volume(void *disk_ptr, uint32_t start_sector, uint32_t sector_count);

//This function passes down to the underlying FS driver
void * volmgr_open_file(uint8_t vol_id, const char *filename);
#endif