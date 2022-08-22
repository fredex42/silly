#ifndef __FAT_FS_H
#define __FAT_FS_H

#ifdef __BUILDING_TESTS
#include <stdint.h>
#else
#include <types.h>
#endif

#include "generic_storage.h"
#include "vfat.h"

/*
The FATFS struct extends the GenericFS struct which contains only the function definitions to
also encapsulate the filesystem data
*/
typedef struct fat_fs {
  void (*mount)(struct fat_fs *fs_ptr, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));
  void (*unmount)(struct fat_fs *fs_ptr);
  //void (*find_file)(struct fat_fs *fs_ptr, char *path, void (*callback)(struct fat_fs *fs_ptr, DirectoryEntry *entry));

  void (*did_mount_cb)(struct fat_fs *fs_ptr, uint8_t status, void *extradata);
  void *did_mount_cb_extradata;

  struct generic_storage_driver *storage;

  uint8_t busy;

  uint8_t reserved[16];
  uint8_t drive_nr;
  BootSectorStart *start;
  BIOSParameterBlock *bpb;
  ExtendedBiosParameterBlock *ebpb;
  FAT32ExtendedBiosParameterBlock *f32bpb;
  FSInformationSector *infosector;
  uint32_t reserved_sectors;
  struct vfat_cluster_map *cluster_map;

  struct mount_transient_data *mount_data_ptr;

} FATFS;

/* Public functions */
FATFS* new_fat_fs(uint8_t drive_nr);

typedef struct mount_transient_data {
  void *extradata;
  void (*mount_completed_cb)(struct fat_fs *fs_ptr, uint8_t status, void *extradata);

} MOUNT_TRANSIENT_DATA;

#define BYTES_PER_CLUSTER(fs_ptr) ((uint32_t)fs_ptr->bpb->bytes_per_logical_sector * (uint32_t)fs_ptr->bpb->logical_sectors_per_cluster)
#define SECTOR_FOR_CLUSTER(fs_ptr, cluster_num) ((uint32_t)cluster_num * (uint32_t)fs_ptr->bpb->logical_sectors_per_cluster)
#endif
