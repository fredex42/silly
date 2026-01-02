#ifndef __FAT_FS_H
#define __FAT_FS_H

#ifdef __BUILDING_TESTS
#include <stdint.h>
#else
#include <types.h>
#endif

#include <drivers/generic_storage.h>
#include "vfat.h"

/*
The FATFS struct extends the GenericFS struct which contains only the function definitions to
also encapsulate the filesystem data
*/
typedef struct fat_fs {
  void (*mount)(struct fat_fs *fs_ptr, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));
  void (*unmount)(struct fat_fs *fs_ptr);

  struct generic_storage_driver *storage;

  uint8_t busy;

  uint8_t reserved[16];
  size_t open_file_count;
  BootSectorStart *start;
  BIOSParameterBlock *bpb;
  ExtendedBiosParameterBlock *ebpb;
  FAT32ExtendedBiosParameterBlock *f32bpb;
  FSInformationSector *infosector;
  uint32_t reserved_sectors;
  struct vfat_cluster_map *cluster_map;

  struct vfat_directory_cache *directory_cache;

  struct VolMgr_Volume *volume;
} FATFS;

/* Public functions */
FATFS* new_fat_fs(uint8_t drive_nr);
void vfat_mount(FATFS *new_fs, void *volmgr_volume, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));

typedef struct mount_transient_data {
  void *extradata;
  size_t sector_load_start;
  size_t sectors_to_load;
  void (*mount_completed_cb)(struct fat_fs *fs_ptr, uint8_t status, void *extradata);

} MOUNT_TRANSIENT_DATA;

#define BYTES_PER_CLUSTER(fs_ptr) ((uint32_t)fs_ptr->bpb->bytes_per_logical_sector * (uint32_t)fs_ptr->bpb->logical_sectors_per_cluster)
#define SECTOR_FOR_CLUSTER(fs_ptr, cluster_num) ((uint32_t)cluster_num * (uint32_t)fs_ptr->bpb->logical_sectors_per_cluster)

#define ERR_FS_BUSY   1


#endif
