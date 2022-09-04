#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
#include <stdio.h>  //for printf()
#endif

#include "fat_fs.h"
#include "vfat_directory_cache.h"
#include "file.h"

void scan_directory(FATFS *fs_ptr, uint32_t starting_cluster, VFAT_DIRECTORY_CACHE_DATA *data, void (*callback)(FATFS *fs_ptr, uint8_t status,VFAT_DIRECTORY_CACHE_DATA *data))
{

}

#define BUFFER_SIZE_IN_PAGES 16
#define PAGE_SIZE 4096

void root_directory_loaded_cb(FATFS *fs_ptr, uint8_t status, void *buffer)
{
  if(status!=0) {
    printf("ERROR Could not load root directory, error %d", status);
    return;
  }
  printf("Loaded root directory at %llx", (uint64_t)buffer);

  uint32_t directory_count=0;
  DirectoryEntry *entry;
  char shortname[14];
  memset(shortname, 0, 14);
  char attrs[12];
  memset(attrs, 0, 12);

  while(1) {
      entry = (DirectoryEntry *)(buffer + directory_count*sizeof(DirectoryEntry));
      if(entry->short_name[0]==0) {
        printf("DEBUG Came to the end of the directory after %d entries.\n", directory_count);
        break;
      } else {
        memcpy(shortname, entry->short_name, 8);
        shortname[9]='.';
        memcpy(&shortname[10], entry->short_xtn, 3);
        shortname[13]=0;
        decode_attributes(entry->attributes, attrs);
        printf("INFO Got %s, size %d, attributes %s\n", shortname, entry->file_size, attrs);
        ++directory_count;
      }
  }
}

void initialise_directory_cache(FATFS *fs_ptr,
   void (*callback)(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root))
{
  printf("Caching directory tables...\n");

  void *buffer = malloc(BUFFER_SIZE_IN_PAGES*PAGE_SIZE);

  if(fs_ptr->f32bpb) {
    uint32_t root_directory_entry_cluster = fs_ptr->f32bpb->root_directory_entry_cluster;
    printf("Found FAT32root directory at cluster %ld\n", root_directory_entry_cluster);
    fat_load_file(fs_ptr, root_directory_entry_cluster, buffer, BUFFER_SIZE_IN_PAGES*PAGE_SIZE, &root_directory_loaded_cb);
  } else {
    printf("FAT12/FAT16 not implemented yet");
  }


}
