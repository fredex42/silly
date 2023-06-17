#include <unistd.h>
#include "vfat.h"

#ifndef __COPIER_H
#define __COPIER_H

size_t get_file_size(char *source_file_name);
int f16_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb);
int f32_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb);
#endif