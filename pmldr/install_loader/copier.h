#include <unistd.h>
#include "vfat.h"

#ifndef __COPIER_H
#define __COPIER_H

struct copied_file_info {
    uint16_t start_sector_location;
    uint32_t length_in_bytes;
    uint16_t length_in_sectors;
};

size_t get_file_size(char *source_file_name);
int f16_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb,  struct copied_file_info *dest_info);
int f32_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, struct copied_file_info *dest_info);
uint32_t f32_find_free_clusters(uint32_t required_length, uint32_t *allocation_table, size_t table_size);

int recursive_copy_file(int source_fd, int raw_device_fd, size_t dest_block_offset, size_t total_blocks, size_t reserved_bytes, size_t block_size);
int copy_next_block(int source_fd, size_t blocks_copied, size_t total_blocks, int raw_device_fd, size_t dest_block, size_t reserved_bytes, size_t block_size, void *buffer);
#endif