#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
#include <stdio.h>  //for printf()
#endif

#include "generic_storage.h"
#include "fat_fs.h"
#include "cluster_map.h"
#include "ucs_conv.h"
#include "string_helpers.h"
#include "vfat_directory_cache.h"

/**
Step 4. Directory tree is now cached so we can return to caller.
*/
void directory_cache_loaded(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root)
{
  printf("Completed caching in directories\n");
  void (*final_callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata) = fs_ptr->mount_data_ptr->mount_completed_cb;
  void *cb_extradata = fs_ptr->mount_data_ptr->extradata;
  free(fs_ptr->mount_data_ptr);
  fs_ptr->mount_data_ptr = NULL;
  fs_ptr->busy = 0;
  final_callback(fs_ptr, status, cb_extradata);
}

/**
Step 3. Called once the cluster map has been loaded.
*/
void cluster_map_loaded(FATFS *fs_ptr, uint8_t status, VFatClusterMap *map) {
  if(map==NULL) {
    printf("ERROR could not load FAT cluster map, status is %d\n", status);
    fs_ptr->busy=0;
    //fs_ptr->did_mount_cb(fs_ptr, 2, fs_ptr->mount_data_ptr);
  } else {
    fs_ptr->cluster_map = map;
    printf("Found a FAT%d filesystem\n", map->bitsize);
  }

  fs_ptr->cluster_map = map;
  initialise_directory_cache(fs_ptr, &directory_cache_loaded);
}

/**
Step 2. Called once we have read in the first block (512 bytes) of the filesystem
*/
void read_header_block_complete(uint8_t status, void *buffer, void *p, void *extradata)
{
  char is_fat32=0;
  FATFS *fs_ptr = (FATFS *)p;

  fs_ptr->start = (BootSectorStart *)buffer;
  fs_ptr->bpb = (BIOSParameterBlock *)(buffer + 0x00B);
  printf("Mounting FAT FS with %d bytes per sector, %d sectors per cluster, %d FATs\n",
    (int)fs_ptr->bpb->bytes_per_logical_sector,
    (int)fs_ptr->bpb->logical_sectors_per_cluster,
    (int)fs_ptr->bpb->fat_count);

  if(fs_ptr->bpb->max_root_dir_entries==0 && fs_ptr->bpb->total_logical_sectors==0) {
    is_fat32=1;
    printf("FS is probably FAT32\n");
  } else {
    printf("FS is probably not FAT32\n");
  }

  if(is_fat32) {
    fs_ptr->ebpb = NULL;
    fs_ptr->f32bpb = (FAT32ExtendedBiosParameterBlock *)(buffer + 0x0B + 25);
  } else {
    fs_ptr->ebpb = (ExtendedBiosParameterBlock *)(buffer + 0x0B + 25);
    fs_ptr->f32bpb = NULL;
  }
  vfat_load_cluster_map(fs_ptr, &cluster_map_loaded);
}

/**
Step 1. Called by the initializer to start the mount operation
*/
void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata))
{
  char *block_buffer = (char *)malloc(4096);
  memset(block_buffer, 0, 4096);

  if(fs_ptr->storage==NULL) {
    printf("fs_ptr->storage is not set!\n");
    callback(fs_ptr, 1, extradata);
  } else {
    fs_ptr->busy = 1;
    fs_ptr->did_mount_cb = callback;
    fs_ptr->drive_nr = drive_nr;
    fs_ptr->mount_data_ptr = (MOUNT_TRANSIENT_DATA *)malloc(sizeof(MOUNT_TRANSIENT_DATA));
    fs_ptr->mount_data_ptr->extradata = extradata;
    fs_ptr->mount_data_ptr->mount_completed_cb = callback;
    fs_ptr->storage->driver_start_read(fs_ptr, drive_nr, 0, 1, block_buffer, fs_ptr, &read_header_block_complete);
  }
}
