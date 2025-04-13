#include <types.h>

//This header file contains the internal definitions of the volume manager.
//Consumers should use the version in `include/`.

#ifndef __VOLMGR_H
#define __VOLMGR_H

struct DiskDescriptor {
    uint32_t refcount;
    struct DiskDescriptor *next;
    struct DiskDescriptor *prev;

    //struct DiskDriverIface *driver;
};

struct VolumeDescriptor {
    uint32_t refcount;
    struct VolumeDescriptor *next;
    struct VolumeDescriptor *prev;
    char serial[32];
    
    struct DiskDescriptor *physicalDisk;
    uint8_t preferred_access_method;    //index to the preferred hardware driver descriptor
};

/*
This struct holds the entire state of the volume manager, and is used in every call.
*/
struct VolMgr {
    //Linked list pointing to all available volumes
    struct VolumeDescriptor *volumeList;
    //Linked list pointing to all available physical disks
    struct DiskDescriptor *physicalDiskList;

    uint8_t bios_boot_number;
};

struct VolMgr *volmgr_init(uint8_t bios_disk_number);
struct VolumeDescriptor *volmgr_new_voldescriptor(struct DiskDescriptor *physical_disk);
//TODO - will need more params when interface is more finalised
struct DiskDescriptor *volmgr_new_diskdescriptor();

#endif