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


#endif
