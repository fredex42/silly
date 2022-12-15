#include <types.h>

#ifndef __FS_FAT_FILEOPS_H
#define __FS_FAT_FILEOPS_H

typedef struct vfat_open_file {
  struct fat_fs* parent_fs;
  size_t current_cluster_number;
  size_t sector_offset_in_cluster;

  size_t byte_offset_in_cluster;
  size_t file_length;

  uint8_t busy : 1;
} VFatOpenFile;

void vfat_close(VFatOpenFile *fp);
VFatOpenFile* vfat_open(struct fat_fs *fs_ptr, struct directory_entry* entry_to_open);
VFatOpenFile* vfat_open_by_location(struct fat_fs *fs_ptr, size_t cluster_location_start, size_t file_size);
void vfat_read_async(VFatOpenFile *fp, void* buf, size_t length, void* extradata, void(*callback)(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata));

#endif
