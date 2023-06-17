#ifndef __FS_H
#define __FS_H
#include <sys/types.h>
#include <unistd.h>
#include "vfat.h"

void *load_bootsector(int fd);
void get_oem_name(void *bootsect, char *buf);

#define get_bpb(bs_ptr) (BIOSParameterBlock *)(bs_ptr + 0x0B)
#define get_jump_bytes(bs_ptr) (char *)bs_ptr;  //first 3 bytes
#define OEM_NAME_LENGTH 8

#define boot_sig_byte(bs_ptr,off) ((char *)bs_ptr)[0x1FE + off]

uint16_t file_size_to_clusters(size_t file_size, BIOSParameterBlock *bpb);
DirectoryEntry* f16_get_root_dir(int raw_device_fd, BIOSParameterBlock *bpb);
int f16_write_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, DirectoryEntry *entries);
size_t f16_get_root_dir_location(BIOSParameterBlock *bpb) ;

#endif