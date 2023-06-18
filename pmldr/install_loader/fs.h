#ifndef __FS_H
#define __FS_H
#include <sys/types.h>
#include <unistd.h>
#include "vfat.h"

void *load_bootsector(int fd);
void get_oem_name(void *bootsect, char *buf);

#define get_bpb(bs_ptr) (BIOSParameterBlock *)(bs_ptr + 0x0B)
//NOTE: Check that this _is_ a FAT32 FS (by validating correct 0 values in BPB) before attempting to use the fat32 EBPB
#define get_f32bpb(bs_ptr) (FAT32ExtendedBiosParameterBlock *)(bs_ptr + 0x24)

#define get_jump_bytes(bs_ptr) (char *)bs_ptr;  //first 3 bytes
#define OEM_NAME_LENGTH 8

#define boot_sig_byte(bs_ptr,off) ((char *)bs_ptr)[0x1FE + off]

uint16_t file_size_to_clusters(size_t file_size, BIOSParameterBlock *bpb);

DirectoryEntry* f16_get_root_dir(int raw_device_fd, BIOSParameterBlock *bpb);
int f16_write_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, DirectoryEntry *entries);
size_t f16_get_root_dir_location(BIOSParameterBlock *bpb) ;

DirectoryEntry *f32_get_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb);
int f32_write_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, DirectoryEntry *entries);

void *get_allocation_table(int raw_device_fd, BIOSParameterBlock *bpb);
int write_allocation_table(int raw_device_fd, BIOSParameterBlock *bpb, void *buffer);
#endif