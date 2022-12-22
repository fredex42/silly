#include <types.h>

#ifndef __FS_FAT_FILEOPS_H
#define __FS_FAT_FILEOPS_H

typedef struct vfat_open_file {
  struct fat_fs* parent_fs;
  size_t current_cluster_number;
  size_t sector_offset_in_cluster;
  size_t byte_offset_in_sector;

  size_t byte_offset_in_cluster;
  size_t file_length;
  size_t first_cluster;

  size_t fs_sector_offset;
  uint8_t busy : 1;
} VFatOpenFile;

void vfat_close(VFatOpenFile *fp);
VFatOpenFile* vfat_open(struct fat_fs *fs_ptr, struct directory_entry* entry_to_open);
VFatOpenFile* vfat_open_by_location(struct fat_fs *fs_ptr, size_t cluster_location_start, size_t file_size, size_t cluster_offset);
void vfat_read_async(VFatOpenFile *fp, void* buf, size_t length, void* extradata, void(*callback)(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata));

/**
Sets the internal position of the given open file.
Returns 0 if successful, 1 if EOF was encountered, 2 if the parameters were not valid
*/
uint8_t vfat_seek(VFatOpenFile *fp, size_t offset, uint8_t whence);

#define SEEK_SET 1
#define SEEK_CURRENT 2

#endif
