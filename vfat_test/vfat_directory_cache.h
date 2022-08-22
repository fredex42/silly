#ifndef __VFAT_DIRECTORY_CACHE_H
#define __FAT_FS__VFAT_DIRECTORY_CACHE_H

#ifdef __BUILDING_TESTS
#include <stdint.h>
#else
#include <types.h>
#endif

#include "generic_storage.h"
#include "vfat.h"

typedef struct vfat_directory_cache_node {
  struct vfat_directory_cache_node *next;
  uint8_t is_leaf;
  DirectoryEntry *entry;
} VFAT_DIRECTORY_CACHE_NODE;

struct vfat_directory_cache {

};

typedef struct vfat_directory_cache_transient_data {

} VFAT_DIRECTORY_CACHE_DATA;

void initialise_directory_cache(FATFS *fs_ptr, void (*callback)(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root));

#endif
