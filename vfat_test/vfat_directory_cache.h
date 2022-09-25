#ifndef __VFAT_DIRECTORY_CACHE_H
#define __FAT_FS__VFAT_DIRECTORY_CACHE_H

#ifdef __BUILDING_TESTS
#include <stdint.h>
#else
#include <types.h>
#endif

#include "utils/directory_queue.h"
#include "generic_storage.h"
#include "vfat.h"

typedef struct vfat_directory_cache_node {
  struct vfat_directory_cache_node *next;
  struct vfat_directory_cache_node *child;  //child==NULL => leaf node (i.e. file)
  char full_file_name[255];
  DirectoryEntry entry;
} VFAT_DIRECTORY_CACHE_NODE;

typedef struct vfat_directory_cache {
  size_t bytes_allocated;
  size_t bytes_used;

  VFAT_DIRECTORY_CACHE_NODE *start;
} VFAT_DIRECTORY_CACHE;

typedef struct vfat_directory_cache_transient_data {
  DIRECTORY_QUEUE *dirs_to_process;
} VFAT_DIRECTORY_CACHE_DATA;

void initialise_directory_cache(FATFS *fs_ptr, void (*callback)(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root));

#endif
