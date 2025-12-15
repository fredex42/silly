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

//Callback flags
#define CB_ONESHOT 0x80
#define CB_MOUNT 0x01
#define CB_UNMOUNT 0x02
#define CB_DISK_ADDED 0x04
#define CB_DISK_REMOVED 0x08

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

/**
 * Registers a callback to be invoked when a volume on the given target is mounted.
 * Strings are copied into the callback structure so they do not need to remain valid after this call.
 * The extradata pointer is NOT copied and must remain valid until the callback is invoked.
 * The target may be a disk or a volume name.
 * The optional label is for debugging purposes only.
 */
void volmgr_register_callback(char *target, char *opt_label, uint8_t flags, void *extradata, void (*callback)(uint8_t status, const char *target, void *volume, void *extradata));

/**
 * Removes the given callback for the specified target.
 * Returns E_OK if successful, or E_NOT_SUPPORTED if the callback was not found.
 */
uint8_t volmgr_unregister_callback(char *target, void (*callback)(uint8_t status, const char *target, void *volume, void *extradata));

/**
 * Registers the given alias name to point to the specified target device name.
 */
uint8_t volmgr_internal_register_alias(char *alias_name, char *target_name);

/**
 * Internal function to unregister an alias by name.
 * Returns E_OK if successful, E_PARAMS if the alias name was not valid or E_NOT_SUPPORTED if the alias was not found.
 */
uint8_t volmgr_internal_unregister_alias(char *alias_name);
#endif