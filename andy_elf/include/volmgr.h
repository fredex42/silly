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
//This is called by hardware init/scan code including PCI, ACPI and ISA bus code
uint32_t volmgr_add_disk(enum disk_type type, uint32_t base_addr, uint32_t flags);
uint8_t volmgr_get_disk_count();
void *volmgr_disk_for_id(uint32_t id);

void volmgr_vol_unref(struct VolMgr_Volume *vol);
void volmgr_vol_ref(struct VolMgr_Volume *vol);

//Called internally to add a new disk
struct VolMgr_Volume* volmgr_add_volume(void *disk_ptr, uint32_t start_sector, uint32_t sector_count, uint8_t partition_type, uint8_t partition_number);

void volmgr_mount_volume(struct VolMgr_Volume *vol, void *extradata, void (*callback)(uint8_t status, void *fs_ptr, void *extradata));
//This function passes down to the underlying FS driver
void * volmgr_open_file(uint8_t vol_id, const char *filename);

/**
 * Public function that starts a read operation on the given volume pointer
 */
int8_t volmgr_vol_start_read(struct VolMgr_Volume *vol, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));
int8_t volmgr_vol_start_write(struct VolMgr_Volume *vol, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));

/**
 * Returns a pointer to the disk structure with the given name, or NULL if not found
 */
void* volmgr_get_disk_by_name(const char *name);
void volmgr_get_volume_name(struct VolMgr_Volume *vol, char *out_name, size_t max_len);

void *volmgr_resolve_path_to_fs(const char *path);
void *volmgr_resolve_path_to_volume(const char *path);
#endif