#include <types.h>
#include <malloc.h>
#include <spinlock.h>
#include <memops.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <errors.h>
#include <kernel_config.h>
#include <panic.h>
#include <string.h>
#include "volmgr_internal.h"
#include "../drivers/ata_pio/ata_pio.h"

struct VolMgr_GlobalState *volmgr_state = NULL;
volatile spinlock_t volmgr_lock = 0;

void volmgr_init(struct KernelConfig *config) {
    kputs("volmgr: Initialising Volume Manager\r\n");
    volmgr_state = (struct VolMgr_GlobalState *)malloc(sizeof(struct VolMgr_GlobalState));
    kprintf("volmgr: Allocated global state at 0x%x\r\n", volmgr_state);
    kprintf("volmgr: spinlock at 0x%x\r\n", &volmgr_lock);
    memset(volmgr_state, 0, sizeof(struct VolMgr_GlobalState));
    volmgr_state->disk_list = NULL;
    volmgr_state->disk_count = 0;
    volmgr_state->internal_counter = 0;
    if(config!=NULL) {
        volmgr_state->root_device = config_root_device(config);
    } else {
        kputs("WARNING: volmgr was not supplied with kernel configuration\r\n");
        volmgr_state->root_device = (char *)malloc(8);
        if(!volmgr_state->root_device) {
            k_panic("Not enough memory to store root device string");
            return; //yeah this won't actually happen as 'k_panic' locks us
        }
        kputs("Falling back to $ide0p0");
        strncpy(volmgr_state->root_device, "$ide0p0", 8);
    }
    volmgr_lock = 0;
}

/*
(Internal) - Called when the vfat_mount operation in volmgr_mount_volume completes, and relays information back to the original caller
*/
void vol_mounted_cb(FATFS *fs_ptr, uint8_t status, void *extradata) {
    kputs("volmgr: Volume mounted callback invoked\r\n");
    if(extradata==NULL) {
        kputs("volmgr: vol_mounted_cb called with NULL extradata\r\n");
        if(fs_ptr) free(fs_ptr);
        return;
    }

    struct volmgr_internal_mount_data *mount_data = (struct volmgr_internal_mount_data *)extradata;
    switch(status) {
        case E_OK:
            kprintf("volmgr: Volume 0x%x mounted successfully: FS ptr 0x%x\r\n", mount_data->volume, fs_ptr);
            mount_data->volume->fs_ptr = fs_ptr;
            mount_data->callback(E_OK, fs_ptr, mount_data);
            free(mount_data);
            break;
        default:
            kprintf("volmgr: Volume 0x%x mount failed with status %d\r\n", mount_data->volume, status);
            if(fs_ptr) free(fs_ptr);
            void *extradata = mount_data->extradata;
            mount_data->callback(status, NULL, mount_data);
            free(mount_data);
            break;
    }
}

/**
 * Called when the mount operation for a volume completes.
 */
void mount_volume_completed(uint8_t status, void *fs_ptr, void *extradata) {
    kputs("volmgr: mount_volume_completed invoked\r\n");
    if(extradata==NULL) {
        kputs("volmgr: mount_volume_completed called with NULL extradata\r\n");
        if(fs_ptr) free(fs_ptr);
        return;
    }
    struct volmgr_internal_mount_data *mount_data = (struct volmgr_internal_mount_data *)extradata;

    if(status==E_OK) {
        kprintf("volmgr: Volume %s mounted successfully\r\n", mount_data->volume->name);
        mount_data->volume->fs_ptr = fs_ptr;

        volmgr_internal_trigger_callbacks(CB_MOUNT, E_OK, mount_data->volume->name, mount_data->volume);

        if(strncmp(mount_data->volume->name, volmgr_state->root_device, 8)==0) {
            kputs("volmgr: This is the root device, mounting as root filesystem\r\n");
            //This is the root device, set up any necessary root FS pointers here
            volmgr_internal_register_alias("#root", mount_data->volume->name);
            volmgr_internal_trigger_callbacks(CB_MOUNT, E_OK, "#root", mount_data->volume);
        }
    } else {
        kprintf("volmgr: Volume %s mount failed with status %d\r\n", mount_data->volume->name, status);
        volmgr_internal_trigger_callbacks(CB_MOUNT, status, mount_data->volume->name, mount_data->volume);
    }
}

void volmgr_mount_volume(struct VolMgr_Volume *vol, void *extradata, void (*callback)(uint8_t status, void *fs_ptr, void *extradata))
{
    struct volmgr_internal_mount_data *mount_data = (struct volmgr_internal_mount_data *)malloc(sizeof(struct volmgr_internal_mount_data));
    if(!mount_data) {
        kputs("volmgr: Unable to allocate memory for mount data\r\n");
        callback(E_NOMEM, NULL, extradata);
        return;
    }

    memset(mount_data, 0, sizeof(struct volmgr_internal_mount_data));
    mount_data->extradata = extradata;
    mount_data->callback = callback;
    mount_data->volume = vol;

    switch(vol->part_type) {
        case PARTTYPE_FAT12:
        case PARTTYPE_FAT16_SMALL:
        case PARTTYPE_FAT16:
        case PARTTYPE_FAT32:
        case PARTTYPE_FAT32_LBA:
        case PARTTYPE_FAT16_LBA:
            //Mount FAT filesystem
            kputs("volmgr: Mounting FAT filesystem\r\n");
            struct fat_fs *new_fs = (struct fat_fs *)malloc(sizeof(struct fat_fs));
            if(!new_fs) {
                kputs("volmgr: Unable to allocate memory for FATFS structure\r\n");
                callback(E_NOMEM, NULL, extradata);
                return;
            }
            memset(new_fs, 0, sizeof(struct fat_fs));

            //Start the mount operation.  On completion, vol_mounted_cb will be called with either an error code or the populated new_fs
            vfat_mount((FATFS *)new_fs, (void *)vol, (void *)mount_data, &vol_mounted_cb);
            break;
        case PARTTYPE_NTFS:
            //Mount NTFS filesystem
            kputs("volmgr: NTFS mounting not yet implemented\r\n");
            callback(NULL, E_NOT_SUPPORTED,mount_data);
            break;
        default:
            kprintf("volmgr: Unsupported partition type 0x%x\r\n", vol->part_type);
            callback(NULL, E_NOT_SUPPORTED, mount_data);
            break;
    }
}

struct VolMgr_Volume* volmgr_add_volume(void *disk_ptr, uint32_t start_sector, uint32_t sector_count, uint8_t partition_type, uint8_t partition_number) {
    struct VolMgr_Disk *disk = (struct VolMgr_Disk *)disk_ptr;
    if(!disk) {
        kputs("volmgr: volmgr_add_volume called with NULL disk pointer\r\n");
        return NULL;
    }
    struct VolMgr_Volume *new_volume = (struct VolMgr_Volume *)malloc(sizeof(struct VolMgr_Volume));
    if(!new_volume) {
        kputs("volmgr: Unable to allocate memory for new volume\r\n");
        return NULL;
    }
    memset(new_volume,0, sizeof(struct VolMgr_Volume));
    new_volume->start_sector = start_sector;
    new_volume->sector_count = sector_count;
    volmgr_disk_ref(disk);  //The volume holds a strong ref to the disk
    acquire_spinlock(&volmgr_lock);
    new_volume->disk = disk;
    new_volume->next = disk->volumes;
    new_volume->part_type = (enum PartitionType)partition_type;
    new_volume->fs_ptr = NULL; //To be initialised when the FS is mounted
    strncpy(new_volume->name, disk->base_name, 8);
    new_volume->name[5] = 'p';
    new_volume->name[6] = '0' + partition_number;
    disk->volumes = new_volume;
    disk->volume_count++;
    release_spinlock(&volmgr_lock);
    kprintf("volmgr: Added volume to disk 0x%x: Start Sector %d, Sector Count %d\r\n", disk, start_sector, sector_count);
    return new_volume;
}


/**
 * Public function that starts a read operation on the given volume pointer
 */
int8_t volmgr_vol_start_read(struct VolMgr_Volume *vol, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
    if(!vol || !buffer || sector_count==0) {
        return E_PARAMS;
    }
    //kprintf("volmgr: volmgr_vol_start_read: Volume 0x%x, LBA 0x%x, Sector Count 0x%x\r\n", vol, (uint32_t)lba_address, sector_count);
    uint64_t physical_lba = lba_address + vol->start_sector;
    return volmgr_disk_start_read(vol->disk, physical_lba, sector_count, buffer, extradata, callback);
}

int8_t volmgr_vol_start_write(struct VolMgr_Volume *vol, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
    if(!vol || !buffer || sector_count==0) {
        return E_PARAMS;
    }
    uint64_t physical_lba = lba_address + vol->start_sector;
    return volmgr_disk_start_write(vol->disk, physical_lba, sector_count, buffer, extradata, callback);
}

uint32_t volmgr_next_counter_value() {
    acquire_spinlock(&volmgr_lock);
    uint32_t val = volmgr_state->internal_counter;
    volmgr_state->internal_counter++;
    release_spinlock(&volmgr_lock);
    return val;
}

uint8_t volmgr_get_disk_count() {
    return volmgr_state->disk_count;
}

/**
 * Internal function, called by the IO driver when the bootsector has been
 *  loaded into memory.  This parses the partition table and adds volumes
 *  accordingly.
 *  */
void volmgr_boot_sector_loaded(uint8_t status, void *buffer, void *extradata) {
    struct VolMgr_Disk *disk = (struct VolMgr_Disk *)extradata;
    kprintf("volmgr: Boot sector loaded callback for 0x%x\r\n", disk);
    //Parse partition table from buffer
    uint8_t *boot_sector = (uint8_t *)buffer;
    if(boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        kputs("volmgr: Invalid boot sector signature\r\n");
        free(buffer);
        volmgr_disk_unref(disk);
        return;
    }
    disk->optional_signature = (uint32_t)boot_sector[0x1B8];
    kprintf("volmgr: Disk signature: 0x%x\r\n", disk->optional_signature);
    
    for(int i=0; i<4; i++) {
        uint32_t base_offset = 0x1BE + (i * 16);
        //uint8_t *part_entry = &boot_sector[0x1BE + (i * 16)];
        uint8_t part_type = boot_sector[base_offset + 4];
        if(part_type == PARTTYPE_EMPTY)
            continue;
        for(int j=0; j<16; j++) {
            kprintf(" %x", (uint32_t)(boot_sector[base_offset + j] & 0xFF));
        }
        uint32_t start_sector = *(uint32_t *)&boot_sector[base_offset + 8];
        uint32_t sector_count = *(uint32_t *)&boot_sector[base_offset + 12];
        kprintf("\r\nvolmgr: Found partition %d: Type 0x%x, Start Sector %l, Sector Count %l\r\n", i+1, (uint32_t)part_type, start_sector, sector_count);
        struct VolMgr_Volume *vol = volmgr_add_volume(disk, start_sector, sector_count, part_type, i);
        if(!vol) {
            kputs("volmgr: Error adding volume\r\n");
            continue;
        }
        if(sector_count==0) {
            kputs("volmgr: Warning - volume has zero sectors, skipping mount\r\n");
            continue;
        }
        disk->partition_count++;
        volmgr_mount_volume(vol, NULL, &mount_volume_completed);
    }
    free(buffer);
    volmgr_disk_unref(disk);
}

/**
 * Uses the flags and base IO address to determine the ISA IDE disk number.
 * Returns 0xFF if the IO address and flags do not indicate ISA IDE.
 */
uint8_t volmgr_isa_disk_number(struct VolMgr_Disk *disk) {
    if(disk->base_addr==0x1F0 && (disk->flags & DF_IDE_MASTER))
        return 0;
    else if(disk->base_addr==0x1F0 && (disk->flags & DF_IDE_SLAVE))
        return 1;
    else if(disk->base_addr==0x170 && (disk->flags & DF_IDE_MASTER))
        return 2;
    else if(disk->base_addr==0x170 && (disk->flags & DF_IDE_SLAVE))
        return 3;
    else {
        kprintf("volmgr: Unknown IDE disk base address 0x%X\r\n", disk->base_addr);
        return 0xFF;
    }
}

uint8_t volmgr_initialise_disk(struct VolMgr_Disk *disk) {
    switch(disk->type) {
        case DISK_TYPE_ISA_IDE:
            //Initialise ISA IDE disk
            kprintf("volmgr: Initialising ISA IDE disk at 0x%x\r\n", disk);
            void * buffer = malloc(512); //Temporary buffer for boot sector
            uint8_t disk_num = volmgr_isa_disk_number(disk);
            strncpy(disk->base_name, "$ide", 8);
            disk->base_name[4] = '0' + disk_num;
            disk->base_name[5] = '\0';
            kprintf("volmgr: initialising disk %s\r\n", disk->base_name);
            int8_t rc = volmgr_disk_start_read(disk, 0, 1, buffer, (void *)disk, &volmgr_boot_sector_loaded);
            if(rc != E_OK) {
                kprintf("volmgr: Error starting boot sector read for disk 0x%x: %d\r\n", disk, rc);
                free(buffer);
                volmgr_disk_unref(disk);
                return rc;
            }
            return 0;
        case DISK_TYPE_PCI_IDE:
            //Initialise PCI IDE disk
            kputs("volmgr: PCI IDE disk initialisation not yet implemented\r\n");
            return 0xFF;
        default:
            kprintf("volmgr: Unknown disk type %d\r\n", disk->type);
            return 0xFF;
    }
}

/**
 * Internal function to start a read operation using the correct underlying storage driver.
 * You should not call this directly, rather call vol_start_read which will correctly set lba_address
 * with partition offsets
 */
int8_t volmgr_disk_start_read(struct VolMgr_Disk *disk, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
    if (!disk || !buffer || sector_count == 0) {
        return E_PARAMS;
    }

    //kprintf("volmgr: volmgr_disk_start_read: Disk 0x%x, LBA 0x%x, Sector Count 0x%x\r\n", disk, (uint32_t)lba_address, sector_count);
    //kprintf("volmgr: Disk type: 0x%x\r\n", disk->type);

    switch(disk->type) {
        case DISK_TYPE_ISA_IDE:
            uint8_t disk_num = volmgr_isa_disk_number(disk);
            if(disk_num>4) {
                kprintf("ERROR: invalid flags for ISA disk 0x%x\r\n", disk);
                return E_PARAMS;
            }
            volmgr_disk_ref(disk);  //Reference for the async read
            int8_t rc = ata_pio_start_read(disk_num, lba_address, sector_count, buffer, extradata, callback);
            if(rc==E_BUSY) {
                //Enqueue the operation for when the disk is free
                kputs("volmgr: Disk is busy, queuing read operation\r\n");
                uint8_t rc2 = volmgr_disk_stash_pending_operation(disk, VOLMGR_OP_READ, buffer, extradata, sector_count, lba_address, callback);
                if(rc2 != E_OK) {
                    kprintf("volmgr: Failed to queue read operation for disk 0x%x\r\n", disk);
                }
                return rc2;
            }
            return rc;
        case DISK_TYPE_PCI_IDE:
            kputs("ERROR: volmgr_disk_read not yet implemented for PCI IDE\r\n");
            return E_INVALID_DEVICE;
        default:
            kprintf("ERROR: volmr_disk_read invalid disk type 0x%x for 0x%x\r\n", disk->type, disk);
            return E_INVALID_DEVICE;
    }
}

/**
 * Internal function to start a write operation using the correct underlying storage driver.
 * You should not call this directly, rather call vol_start_write which will correctly set lba_address
 * with partition offsets
 */
int8_t volmgr_disk_start_write(struct VolMgr_Disk *disk, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
    if (!disk || !buffer || sector_count == 0) {
        return E_PARAMS;
    }

    switch(disk->type) {
        case DISK_TYPE_ISA_IDE:
            uint8_t disk_num = volmgr_isa_disk_number(disk);
            if(disk_num>4) {
                kprintf("ERROR: invalid flags for ISA disk 0x%x\r\n", disk);
                return E_PARAMS;
            }
            int8_t rc = ata_pio_start_write(disk_num, lba_address, sector_count, buffer, extradata, callback);
            return rc;
        case DISK_TYPE_PCI_IDE:
            kputs("ERROR: volmgr_disk_write not yet implemented for PCI IDE\r\n");
            return E_INVALID_DEVICE;
        default:
            kprintf("ERROR: volmr_disk_write invalid disk type 0x%x for 0x%x\r\n", disk->type, disk);
            return E_INVALID_DEVICE;
    }
}

void volmgr_vol_ref(struct VolMgr_Volume *vol) {
    acquire_spinlock(&volmgr_lock);
    vol->refcount++;
    release_spinlock(&volmgr_lock);
}
void volmgr_vol_unref(struct VolMgr_Volume *vol) {
    acquire_spinlock(&volmgr_lock);
    if(vol->refcount>0) {
        vol->refcount--;
    } else {
        kprintf("volmgr: Freeing volume 0x%x\r\n", vol);
        free(vol->fs_ptr);
        volmgr_disk_unref(vol->disk);
        free(vol);
    }
    release_spinlock(&volmgr_lock);
}

void volmgr_disk_ref(struct VolMgr_Disk *disk) {
    acquire_spinlock(&volmgr_lock);
    disk->refcount++;
    release_spinlock(&volmgr_lock);
}

void volmgr_disk_unref(struct VolMgr_Disk *disk) {
    acquire_spinlock(&volmgr_lock);
    if(disk->refcount>0) {
        disk->refcount--;
    } else {
        for(struct VolMgr_Volume *vol = disk->volumes; vol!=NULL; vol=vol->next) {
            kprintf("volmgr: Unref volume 0x%x from disk 0x%x\r\n", vol, disk);
            volmgr_vol_unref(vol);
        }
        free(disk);
    }
    release_spinlock(&volmgr_lock);
}

void *volmgr_get_volume_by_name(const char *name) {
    char maybe_marker = name[5];
    if(maybe_marker != 'p' && maybe_marker != 'P') {
        //Not a valid volume name
        return NULL;
    }
    char maybe_partnum_char = name[6];
    if(maybe_partnum_char < '0' || maybe_partnum_char > '9') {
        //Not a valid volume name
        return NULL;
    }

    uint8_t partnum = (uint8_t)(maybe_partnum_char - '0');
    char disk_name[8];
    strncpy(disk_name, name, 6);
    struct VolMgr_Disk *disk = (struct VolMgr_Disk *)volmgr_get_disk_by_name(disk_name);

    if(!disk) {
        kprintf("volmgr: unable to find disk with name %s\r\n", disk_name);
        return NULL;
    }

    acquire_spinlock(&volmgr_lock);
    if(partnum >= disk->partition_count) {
        kprintf("volmgr: requested partition %d but disk only has %d partitions\r\n", partnum, disk->partition_count);
        release_spinlock(&volmgr_lock);
        return NULL;
    }
    for(struct VolMgr_Volume *vol = disk->volumes; vol!=NULL; vol=vol->next) {
        if(strncmp(vol->name, name, 8)==0) {
            release_spinlock(&volmgr_lock);
            return (void *)vol;
        }
    }
    release_spinlock(&volmgr_lock);
    return NULL;
}

uint8_t volmgr_disk_stash_pending_operation(struct VolMgr_Disk *disk, enum PendingOperationType type, void *buffer, void *extradata, uint16_t sector_count, uint64_t lba_address, void (*callback)(uint8_t status, void *buffer, void *extradata)) {
    acquire_spinlock(&volmgr_lock);
    for(size_t i=0; i<disk->pending_operation_count; i++) {
        if(disk->pending_operations[i].type == VOLMGR_OP_NONE) {
            disk->pending_operations[i].type = type;
            disk->pending_operations[i].buffer = buffer;
            disk->pending_operations[i].extradata = extradata;
            disk->pending_operations[i].sector_count = sector_count;
            disk->pending_operations[i].lba_address = lba_address;
            disk->pending_operations[i].callback = callback;
            release_spinlock(&volmgr_lock);
            return E_OK;
        }
    }
    kputs("volmgr: Warning - no free pending operation slots available\r\n");
    disk->pending_operation_count+=10;
    disk->pending_operations = (struct VolMgr_PendingOperation *)realloc(disk->pending_operations, sizeof(struct VolMgr_PendingOperation) * disk->pending_operation_count);
    if(disk->pending_operations == NULL) {
        kputs("volmgr: Error reallocating pending operations array\r\n");
        release_spinlock(&volmgr_lock);
        return E_NOMEM;
    }
    memset(&disk->pending_operations[disk->pending_operation_count - 10], 0, sizeof(struct VolMgr_PendingOperation) * 10);
    disk->pending_operations[disk->pending_operation_count - 10].type = type;
    disk->pending_operations[disk->pending_operation_count - 10].buffer = buffer;
    disk->pending_operations[disk->pending_operation_count - 10].extradata = extradata;
    disk->pending_operations[disk->pending_operation_count - 10].sector_count = sector_count;
    disk->pending_operations[disk->pending_operation_count - 10].lba_address = lba_address;
    disk->pending_operations[disk->pending_operation_count - 10].callback = callback;
    release_spinlock(&volmgr_lock);
    return E_OK;
}

void volmgr_disk_process_pending_operation(struct VolMgr_Disk *disk) {
    acquire_spinlock(&volmgr_lock);
    for(size_t i=disk->pending_operation_hand; i<disk->pending_operation_count; i++) {
        if(disk->pending_operations[i].type != VOLMGR_OP_NONE) {
            struct VolMgr_PendingOperation *op = &disk->pending_operations[i];
            //Found a pending operation
            switch(op->type) {
                case VOLMGR_OP_READ:
                    kputs("volmgr: Processing pending read operation\r\n");
                    release_spinlock(&volmgr_lock);
                    uint8_t rc =  volmgr_disk_start_read(disk, op->lba_address, op->sector_count, op->buffer, op->extradata, op->callback);
                    volmgr_disk_ref(disk);  //Reference for the async read
                    if(rc == E_OK) {
                        //Clear the pending operation slot
                        op->type = VOLMGR_OP_NONE;
                        op->buffer = NULL;
                        op->extradata = NULL;
                        op->sector_count = 0;
                        op->lba_address = 0;
                        op->callback = NULL;
                    } else {
                        kprintf("volmgr: Failed to start pending read operation, rc=0x%x\r\n", rc);
                    }
                    release_spinlock(&volmgr_lock);
                    return; //Only process one at a time
                case VOLMGR_OP_WRITE:
                    kputs("volmgr: Processing pending write operation\r\n");
                    //Similar to read, but for write
                    //Not yet implemented
                    release_spinlock(&volmgr_lock);
                    return;
                default:
                    kputs("volmgr: Unknown pending operation type\r\n");
                    break;
            }
        }
    }
    release_spinlock(&volmgr_lock);
}
/**
 * Returns a pointer to the disk structure with the given name, or NULL if not found
 */
void* volmgr_get_disk_by_name(const char *name) {
    kprintf("debug: volmgr_get_disk_by_name: searching for %s\r\n", name);
    acquire_spinlock(&volmgr_lock);
    for(struct VolMgr_Disk *disk = volmgr_state->disk_list; disk!=NULL; disk=disk->next) {
        kprintf("debug: checking disk %s\r\n", disk->base_name);
        if(strncmp(disk->base_name, name, 8)==0) {
            release_spinlock(&volmgr_lock);
            return (void *)disk;
        }
    }
    release_spinlock(&volmgr_lock);
    return NULL;
}

uint32_t volmgr_add_disk(enum disk_type type, uint32_t base_addr, uint32_t flags) {
    struct VolMgr_Disk *new_disk = (struct VolMgr_Disk *)malloc(sizeof(struct VolMgr_Disk));
    new_disk->type = type;
    new_disk->disk_id = volmgr_next_counter_value();
    new_disk->refcount = 0;
    new_disk->base_addr = base_addr;
    new_disk->flags = flags;
    new_disk->volume_count = 0;
    new_disk->partition_count = 0;
    new_disk->optional_signature = 0;
    new_disk->volumes = NULL;
    new_disk->pending_operation_count = 10;
    new_disk->pending_operation_hand = 0;
    new_disk->pending_operations = (struct VolMgr_PendingOperation *)malloc(sizeof(struct VolMgr_PendingOperation) * new_disk->pending_operation_count);
    memset(new_disk->pending_operations, 0, sizeof(struct VolMgr_PendingOperation) * new_disk->pending_operation_count);

    acquire_spinlock(&volmgr_lock);
    new_disk->next = volmgr_state->disk_list;
    volmgr_state->disk_list = new_disk;
    volmgr_state->disk_count++;
    release_spinlock(&volmgr_lock);
    uint8_t rc = volmgr_initialise_disk(new_disk);
    if(rc != 0) {
        kprintf("volmgr: Warning - disk initialisation returned error code 0x%x\r\n", rc);
    }
    return new_disk->disk_id;
}

void volmgr_get_volume_name(struct VolMgr_Volume *vol, char *out_name, size_t max_len) {
    if(!vol || !out_name || max_len==0) {
        return;
    }
    size_t len = max_len < 8 ? max_len : 8; //name is defined as an 8 byte buffer
    strncpy(out_name, vol->name, len);
}

void *volmgr_resolve_path_to_fs(const char *path) {
    struct VolMgr_Volume *vol = (struct VolMgr_Volume *)volmgr_resolve_path_to_volume(path);
    if(!vol) {
        kprintf("ERROR volmgr_resolve_path_to_fs: could not resolve path %s to volume\r\n", path);
        return NULL;
    }
    if(!vol->fs_ptr) {
        kprintf("ERROR volmgr_resolve_path_to_fs: volume 0x%x has no filesystem mounted\r\n", vol);
        volmgr_vol_unref(vol);
        return NULL;
    }
    void *ptr = vol->fs_ptr;
    volmgr_vol_unref(vol);
    return ptr;
}

/**
 * Returns a pointer to the volume structure for the given path, or NULL if not found.
 * The returned volume has its reference count incremented, and must be unref'd by the caller.
 */
void *volmgr_resolve_path_to_volume(const char *path) {
    const char *path_start = strchr(path, ':');
    if(!path_start) {
        kprintf("ERROR volmgr_resolve_path_to_volume: path %s is not valid (no colon found)\r\n", path);
        return NULL;
    }
    ++path_start;  //skip the colon

    size_t dev_len = (size_t)path_start - (size_t)path;
    char device_name[32];
    strncpy(device_name, path, dev_len > 32 ? 32 : dev_len);
    
    if(path[0] == '#') {
        kprintf("DEBUG: checking alias %s\r\n", device_name);
        //Check for alias
        acquire_spinlock(&volmgr_lock);
        struct VolMgr_Alias *alias = volmgr_state->alias_table;
        while(alias!=NULL) {
            if(strncmp(alias->alias_name, device_name, 32)==0) {
                kprintf("DEBUG: resolved alias %s to device %s\r\n", device_name, alias->device_name);
                strncpy(device_name, alias->device_name, 32);
                break;
            }
            alias = alias->next;
        }
        release_spinlock(&volmgr_lock);
    }

    struct VolMgr_Volume *vol = (struct VolMgr_Volume *)volmgr_get_volume_by_name(device_name);
    if(!vol) {
        kprintf("ERROR volmgr_resolve_path_to_volume: could not resolve path %s to volume\r\n", path);
        return NULL;
    }
    volmgr_vol_ref(vol);
    return (void *)vol;
}

/**
 * Registers the given alias name to point to the specified target device name.
 */
uint8_t volmgr_internal_register_alias(char *alias_name, char *target_name) {
    if(alias_name==NULL || target_name==NULL) {
        return E_PARAMS;
    }
    if(alias_name[0]!='#' || target_name[0]!='$') {
        kputs("volmgr: Alias names must start with '#' and target names with '$'\r\n");
        return E_PARAMS;
    }

    struct VolMgr_Alias *new_alias = (struct VolMgr_Alias *)malloc(sizeof(struct VolMgr_Alias));
    if(!new_alias) {
        kputs("volmgr: Unable to allocate memory for alias\r\n");
        return E_NOMEM;
    }
    memset(new_alias, 0, sizeof(struct VolMgr_Alias));
    strncpy(new_alias->alias_name, alias_name, 32);
    strncpy(new_alias->device_name, target_name, 32);

    acquire_spinlock(&volmgr_lock);
    new_alias->next = volmgr_state->alias_table;
    volmgr_state->alias_table = new_alias;
    release_spinlock(&volmgr_lock);
    return E_OK;
}

/**
 * Internal function to unregister an alias by name.
 * Returns E_OK if successful, E_PARAMS if the alias name was not valid or E_NOT_SUPPORTED if the alias was not found.
 */
uint8_t volmgr_internal_unregister_alias(char *alias_name) {
    if(alias_name==NULL || alias_name[0]!='#') {
        return E_PARAMS;
    }
    acquire_spinlock(&volmgr_lock);
    struct VolMgr_Alias *prev = NULL;
    for(struct VolMgr_Alias *alias = volmgr_state->alias_table; alias!=NULL; alias=alias->next) {
        if(strncmp(alias->alias_name, alias_name, 32)==0) {
            //Found the alias to remove
            if(prev==NULL) {
                volmgr_state->alias_table = alias->next;
            } else {
                prev->next = alias->next;
            }
            free(alias);
            release_spinlock(&volmgr_lock);
            return E_OK;
        }
        prev = alias;
    }
    release_spinlock(&volmgr_lock);
    return E_NOT_SUPPORTED;
}

void volmgr_internal_trigger_callbacks(uint8_t event_flag, uint8_t status, const char *target, void *volume) {
    struct VolMgr_CallbackList *call_list = (struct VolMgr_CallbackList *)malloc(sizeof(struct VolMgr_CallbackList) * 16);
    size_t call_list_count = 0;
    
    acquire_spinlock(&volmgr_lock);
    for(struct VolMgr_CallbackList *cb = volmgr_state->mount_callbacks; cb!=NULL; cb=cb->next) {
        if(strncmp(cb->target, target, 32)==0 && (cb->flags & event_flag)) {
            if(cb->callback==NULL) {
                kputs("volmgr: Warning - callback entry has NULL function pointer, skipping\r\n");
                continue;
            }
            //Copy the callback to a temporary list so we can invoke it outside the lock
            memcpy(&call_list[call_list_count], cb, sizeof(struct VolMgr_CallbackList));
            ++call_list_count;
            if(call_list_count >= 16) {
                kputs("volmgr: Warning - callback list overflow, some callbacks may not be invoked\r\n");
                break;
            }
        }
    }
    release_spinlock(&volmgr_lock);

    for(size_t i=0; i<call_list_count; i++) {
        const struct VolMgr_CallbackList *cb = &call_list[i];
        kprintf("volmgr: Invoking callback 0x%x for target %s (label: %s)\r\n", cb->callback, cb->target, cb->label);
        cb->callback(status, target, volume, cb->extradata);
        if(cb->flags & CB_ONESHOT) {
            kputs("volmgr: Unregistering one-shot callback\r\n");
            volmgr_unregister_callback(cb->target, cb->callback);
        }
    }

    free(call_list);
}

/**
 * Registers a callback to be invoked when a volume on the given target is mounted.
 * Strings are copied into the callback structure so they do not need to remain valid after this call.
 * The extradata pointer is NOT copied and must remain valid until the callback is invoked.
 * The target may be a disk or a volume name.
 * The optional label is for debugging purposes only.
 */
void volmgr_register_callback(char *target, char *opt_label, uint8_t flags, void *extradata, void (*callback)(uint8_t status, const char *target, void *volume, void *extradata)) {
    struct VolMgr_CallbackList *new_cb = (struct VolMgr_CallbackList *)malloc(sizeof(struct VolMgr_CallbackList));
    if(!new_cb) {
        kputs("volmgr: Unable to allocate memory for callback registration\r\n");
        return;
    }
    memset(new_cb, 0, sizeof(struct VolMgr_CallbackList));
    strncpy(new_cb->target, target, 32);
    if(opt_label) {
        strncpy(new_cb->label, opt_label, 12);
    } else {
        memset(new_cb->label, 0, 12);
    }
    new_cb->flags = flags;
    new_cb->callback = callback;
    new_cb->extradata = extradata;

    acquire_spinlock(&volmgr_lock);
    new_cb->next = volmgr_state->mount_callbacks;
    volmgr_state->mount_callbacks = new_cb;
    release_spinlock(&volmgr_lock);
}

/**
 * Removes the given callback for the specified target.
 * Returns E_OK if successful, or E_NOT_SUPPORTED if the callback was not found.
 */
uint8_t volmgr_unregister_callback(char *target, void (*callback)(uint8_t status, const char *target, void *volume, void *extradata)) {
    acquire_spinlock(&volmgr_lock);
    struct VolMgr_CallbackList *prev = NULL;
    for(struct VolMgr_CallbackList *cb = volmgr_state->mount_callbacks; cb!=NULL; cb=cb->next) {
        if(strncmp(cb->target, target, 32)==0 && cb->callback == callback) {
            //Found the callback to remove
            if(prev==NULL) {
                volmgr_state->mount_callbacks = cb->next;
            } else {
                prev->next = cb->next;
            }
            free(cb);
            release_spinlock(&volmgr_lock);
            return E_OK;
        }
        prev = cb;
    }
    release_spinlock(&volmgr_lock);
    return E_NOT_SUPPORTED;
}