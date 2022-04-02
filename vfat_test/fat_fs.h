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
  void (*mount)(struct fat_fs *fs_ptr, uint8_t drive_nr, void (*callback)(uint8_t drive_nr, struct fat_fs *fs_ptr));
  void (*unmount)(struct fat_fs *fs_ptr);
  void (*find_file)(struct fat_fs *fs_ptr, char *path, void (*callback)(struct fat_fs *fs_ptr));

  void (*did_mount_cb)(struct fat_fs *fs_ptr, uint8_t status);

  struct generic_storage_driver *storage;

  uint8_t reserved[16];
  uint8_t drive_nr;
  BootSectorStart *start;
  BIOSParameterBlock *bpb;
  ExtendedBiosParameterBlock *ebpb;
  FAT32ExtendedBiosParameterBlock *f32bpb;
  FSInformationSector *infosector;
  uint32_t reserved_sectors;
  struct vfat_cluster_map *cluster_map;
} FATFS;


uint8_t new_fat_fs(uint8_t drive_nr, void (*callback)(uint8_t drive_nr, FATFS *fs_ptr));
void fat_fs_find_file(struct fat_fs *fs_ptr, char *path, void (*callback)(struct fat_fs *fs_ptr));
void fat_fs_unmount(struct fat_fs *fs_ptr);
void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void (*callback)(uint8_t drive_nr, struct fat_fs *fs_ptr));

#endif
