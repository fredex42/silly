#include "fat_fs.h"

#ifndef __FAT_FILE_H
#define __FAT_FILE_H

typedef struct file_load_transient {
  FATFS *fs_ptr;
  uint32_t buffer_pos;
  uint32_t loading_cluster;
  uint32_t buffer_length_in_clusters;
  void *extradata;
  void (*load_done_cb)(FATFS *fs_ptr, uint8_t status, void *buffer, void *extradata);
} FILE_LOAD_TRANSIENT_DATA;

void fat_load_file(FATFS *fs_ptr, uint32_t starting_cluster, void *buffer, uint32_t buffer_size, void *extradata, void (*load_done_cb)(FATFS *fs_ptr, uint8_t status, void *buffer, void *extradata));
void decode_attributes(uint8_t attrs, char *buf);
#endif
