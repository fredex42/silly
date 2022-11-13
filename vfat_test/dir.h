#ifdef __BUILDING_TESTS
#include <sys/types.h>
#else
#include <types.h>
#endif

#ifndef __VFAT_DIR_H
#define __VFAT_DIR_H


const DirectoryEntry* vfat_peek_next_directory_entry(void *buffer, size_t buffer_size, size_t index);
const DirectoryEntry* vfat_find_in_directory(char *name, char *xtn, void *buffer, size_t buffer_size);
size_t vfat_root_directory_cluster_location(FATFS *fs_ptr);
void split_file_name(char *name, char *stem, char *xtn);

#endif
