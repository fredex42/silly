#include <types.h>
#include <malloc.h>
#include <memops.h>
#include <panic.h>
#include "volmgr.h"

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

