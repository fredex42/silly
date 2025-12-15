#ifndef __VOLMGR_INTERNAL_H
#define __VOLMGR_INTERNAL_H

// Forward declarations for internal use
struct VolMgr_Disk;
struct VolMgr_Volume;

#include <volmgr.h>
#include <fs.h>

enum PendingOperationType {
        VOLMGR_OP_NONE = 0,
        VOLMGR_OP_READ,
        VOLMGR_OP_WRITE
};

struct VolMgr_PendingOperation {
    enum PendingOperationType type;
    void *buffer;
    void *extradata;
    uint16_t sector_count;
    uint64_t lba_address;
    void (*callback)(uint8_t status, void *buffer, void *extradata);
};

struct VolMgr_Disk {
    struct VolMgr_Disk *next;
    uint32_t disk_id;
    uint32_t refcount;
    enum disk_type type;
    uint32_t base_addr;
    uint32_t flags;
    uint8_t volume_count;
    //More fields as needed
    uint32_t optional_signature;
    uint8_t partition_count;
    struct VolMgr_Volume *volumes;
    char base_name[8];
    struct VolMgr_PendingOperation *pending_operations;
    size_t pending_operation_count;
    size_t pending_operation_hand;
};

struct VolMgr_Volume {
    struct VolMgr_Volume *next;
    uint32_t refcount;
    uint32_t start_sector;
    uint32_t sector_count;
    //More fields as needed
    struct VolMgr_Disk *disk;
    enum PartitionType part_type;
    void *fs_ptr; //Pointer to filesystem instance - type is determined by PartitionType
    char name[8];
};

//TODO: implement alias table as a hash table for performance
struct VolMgr_Alias {
    struct VolMgr_Alias *next;
    char alias_name[32];
    char device_name[32];
    struct VolMgr_Volume *volume;
};

typedef (VolMgr_Public_CallbackFunc)(uint8_t status, const char *target, void *volume, void *extradata);

struct VolMgr_CallbackList {
    struct VolMgr_CallbackList *next;
    char target[32]; //Target device or alias name
    char label[12]; //Optional label for debugging
    uint8_t flags;  //CB_XXX flags
    VolMgr_Public_CallbackFunc *callback; 
    void *extradata;
};

struct VolMgr_GlobalState {
    struct VolMgr_Disk *disk_list;
    uint8_t disk_count;
    uint32_t internal_counter;
    char *root_device; //Dynamically allocated string for root device
    struct VolMgr_Alias *alias_table;
    struct VolMgr_CallbackList *mount_callbacks;
};

struct volmgr_internal_mount_data {
    void *extradata;
    struct VolMgr_Volume *volume;
    void (*callback)(uint8_t status, void *fs_ptr, void *extradata);
};

/**
 * Internal function to start a read operation using the correct underlying storage driver.
 * You should not call this directly, rather call vol_start_read which will correctly set lba_address
 * with partition offsets
 */
int8_t volmgr_disk_start_read(struct VolMgr_Disk *disk, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));
int8_t volmgr_disk_start_write(struct VolMgr_Disk *disk, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));

// Function prototypes for internal and external linkage
void kputs(const char *fmt, ...);
void kprintf(const char *fmt, ...);
void volmgr_disk_ref(struct VolMgr_Disk *disk);
void volmgr_disk_unref(struct VolMgr_Disk *disk);
uint8_t volmgr_initialise_disk(struct VolMgr_Disk *disk);
uint8_t volmgr_isa_disk_number(struct VolMgr_Disk *disk);
void vol_mounted_cb(FATFS *fs_ptr, uint8_t status, void *extradata);
uint8_t volmgr_disk_stash_pending_operation(struct VolMgr_Disk *disk, enum PendingOperationType type, void *buffer, void *extradata, uint16_t sector_count, uint64_t lba_address, void (*callback)(uint8_t status, void *buffer, void *extradata));
void volmgr_internal_trigger_callbacks(uint8_t event_flag, uint8_t status, const char *target, void *volume);

#endif