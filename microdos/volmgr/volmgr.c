#include <types.h>
#include <malloc.h>
#include <memops.h>
#include <panic.h>
#include <stdio.h>
#include "volmgr.h"
#include "mbr.h"

//TODO - will need more params when interface is more finalised
struct DiskDescriptor *volmgr_new_diskdescriptor()
{
    struct DiskDescriptor *dd = (struct DiskDescriptor *)malloc(sizeof(struct DiskDescriptor));
    if(!dd) {
        k_panic("Unable to allocate memory for a new disk descriptor");
    }
    memset(dd, 0, sizeof(struct DiskDescriptor));
    dd->refcount = 1;
    return dd;
}

struct VolumeDescriptor *volmgr_new_voldescriptor(struct DiskDescriptor *physical_disk)
{
    struct VolumeDescriptor *vd = (struct VolumeDescriptor *)malloc(sizeof(struct VolumeDescriptor));
    if(!vd) {
        k_panic("Unable to allocate memory for a new disk descriptor");
    }
    memset(vd, 0, sizeof(struct VolumeDescriptor));
    ++physical_disk->refcount;
    vd->physicalDisk = physical_disk;
    vd->refcount = 1;
    return vd;
}

/**
 * Scans the first block of the disk and sets up both the disk descriptor and the
 * volume descriptors.
 * 
 * Returns the number of partitions found on the disk
 */
uint8_t volmgr_init_disk(char *first_disk_block, size_t block_length)
{
    if(block_length<0x200) {
        kprintf("ERROR volmgr unable to init disk, block was short by 0x%x\r\n", 0x200-block_length);
        return 0;
    }
    if(first_disk_block[0x1FE] != 0x55 || first_disk_block[0x1FF] != 0xAA) {
        kputs("ERROR volmgr unable to init disk, block does not appear to be a valid bootsector\r\n");
        return 0;
    }

    struct DiskDescriptor *dd = volmgr_new_diskdescriptor();
    uint32_t optional_disk_id = (uint32_t)first_disk_block[MBR_OFFSET_OPTIONAL_DISKID];
    kprintf("INFO volmgr Found MBR disk with id 0x%x\r\n", optional_disk_id);
    memcpy_dw(dd->disk_identifier, &optional_disk_id, 1);
    
}
struct VolMgr *volmgr_init(uint8_t bios_disk_number)
{
    struct VolMgr *mgr = (struct VolMgr *)malloc(sizeof(struct VolMgr));
    if(mgr==NULL) {
        //does not return
        k_panic("Unable to allocate memory for volume manager");
    }
    memset(mgr, 0, sizeof(struct VolMgr));
    mgr->bios_boot_number = bios_disk_number;
    return mgr;
}

