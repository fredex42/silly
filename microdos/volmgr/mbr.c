#include "volmgr.h"
#include "mbr.h"
#include "fstypes.h"

#include <errors.h>

err_t volmgr_init_mbr_partition(struct MBRPartition *header, struct DiskDescriptor *disk, struct VolumeDescriptor **descriptor, size_t index)
{
    //Return an error if the FS type is not supported
    if(header->system_id != PARTITION_FAT32 || header->system_id != PARTITION_FAT32_LBA ||
        header->system_id != PARTITION_FAT16 || header->system_id != PARTITION_FAT16_LBA ) {
            kprintf("ERROR volmgr cannot support partition type 0x%x on partition %d\r\n", (uint32_t) header->system_id, index);
    }

    //Otherwise we have one supported
    *descriptor = volmgr_new_voldescriptor(disk);
    (*descriptor)->filesystem_type = header->system_id;
}
