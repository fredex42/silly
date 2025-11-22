#include <types.h>
#include <malloc.h>
#include <spinlock.h>
#include <memops.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <errors.h>
#include "volmgr_internal.h"
#include "../drivers/ata_pio/ata_pio.h"
// #include "../drivers/ata_pio/ata_readwrite.h"

struct VolMgr_GlobalState *volmgr_state = NULL;
volatile spinlock_t volmgr_lock = 0;

void volmgr_init() {
    kputs("volmgr: Initialising Volume Manager\r\n");
    volmgr_state = (struct VolMgr_GlobalState *)malloc(sizeof(struct VolMgr_GlobalState));
    kprintf("volmgr: Allocated global state at 0x%x\r\n", volmgr_state);
    kprintf("volmgr: spinlock at 0x%x\r\n", &volmgr_lock);
    memset(volmgr_state, 0, sizeof(struct VolMgr_GlobalState));
    volmgr_state->disk_list = NULL;
    volmgr_state->disk_count = 0;
    volmgr_state->internal_counter = 0;
    volmgr_lock = 0;
}

/*
(Internal) - Called when the vfat_mount operation in volmgr_mount_volume completes, and relays information back to the original caller
*/
void vol_mounted_cb(uint8_t status, void *fs_ptr, void *extradata) {
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
            mount_data->callback(E_OK, fs_ptr, mount_data->extradata);
            free(mount_data);
            break;
        default:
            kprintf("volmgr: Volume 0x%x mount failed with status %d\r\n", mount_data->volume, status);
            if(fs_ptr) free(fs_ptr);
            void *extradata = mount_data->extradata;
            mount_data->callback(status, NULL, mount_data->extradata);
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
        kprintf("volmgr: Volume mounted successfully: FS ptr 0x%x\r\n", fs_ptr);
        mount_data->volume->fs_ptr = fs_ptr;
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
            callback(0xFF, NULL, mount_data);
            break;
        default:
            kprintf("volmgr: Unsupported partition type 0x%x\r\n", vol->part_type);
            callback(0xFF, NULL, mount_data);
            break;
    }
}

struct VolMgr_Volume* volmgr_add_volume(void *disk_ptr, uint32_t start_sector, uint32_t sector_count, uint8_t partition_type) {
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
        uint8_t *part_entry = &boot_sector[0x1BE + (i * 16)];
        uint8_t part_type = part_entry[4];
        if(part_type == PARTTYPE_EMPTY)
            continue;
        uint32_t start_sector = (uint32_t)part_entry[8];
        uint32_t sector_count = (uint32_t)part_entry[12];
        kprintf("volmgr: Found partition %d: Type 0x%x, Start Sector %d, Sector Count %d\r\n", i+1, part_type, start_sector, sector_count);
        struct VolMgr_Volume *vol = volmgr_add_volume(disk, start_sector, sector_count, part_type);
        if(!vol) {
            kputs("volmgr: Error adding volume\r\n");
            continue;
        }
        disk->partition_count++;
        volmgr_mount_volume(vol, NULL, &vol_mounted_cb);
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
            kprintf("volmgr: ISA IDE disk number %d\r\n", disk_num);
            volmgr_disk_ref(disk);  //Reference for the async read
            int8_t rc = ata_pio_start_read(disk_num, 0, 1, buffer, (void *)disk, &volmgr_boot_sector_loaded);
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

    switch(disk->type) {
        case DISK_TYPE_ISA_IDE:
            uint8_t disk_num = volmgr_isa_disk_number(disk);
            if(disk_num>4) {
                kprintf("ERROR: invalid flags for ISA disk 0x%x\r\n", disk);
                return E_PARAMS;
            }
            int8_t rc = ata_pio_start_read(disk_num, lba_address, sector_count, buffer, extradata, callback);
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
        //FIXME - check for dependent objects to free
        free(disk);
    }
    release_spinlock(&volmgr_lock);
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