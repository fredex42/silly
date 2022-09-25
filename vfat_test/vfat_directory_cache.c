#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
#include <stdio.h>  //for printf()
#endif

#include "fat_fs.h"
#include "vfat_directory_cache.h"
#include "file.h"

#define BUFFER_SIZE_IN_PAGES 16
#define PAGE_SIZE 4096

/**
Allocates another entry in the directory cache.
The directory cache is a slab of RAM which contains pointers to specific slices
of the slab.
If necessary, requests another page of RAM from the system to hold it.
If the allocation fails for some reason, returns NULL
*/
VFAT_DIRECTORY_CACHE_NODE *vfat_directory_cache_new_node(VFAT_DIRECTORY_CACHE *c)
{
  if(c->bytes_used+sizeof(VFAT_DIRECTORY_CACHE_NODE) > c->bytes_allocated) {
    printf("INFO Expanding the directory cache by 1 page\n");
    c->start = (VFAT_DIRECTORY_CACHE_NODE *)realloc(c->start, PAGE_SIZE);
    if(c->start==NULL) {
      printf("ERROR Oh no! out of memory.\n");
      return NULL;
    }
    c->bytes_allocated+=PAGE_SIZE;
  }

  //gcc does pointer arithmatic in elements, not bytes.
  //because `c->start` is typed as VFAT_DIRECTORY_CACHE_NODE, adding one to `c->start`
  //means that we add one _element_, i.e. 280 bytes
  VFAT_DIRECTORY_CACHE_NODE *newnode = (c->start + 1);
  c->bytes_used += sizeof(VFAT_DIRECTORY_CACHE_NODE);
  return newnode;
}

void scan_directory(FATFS *fs_ptr, uint32_t starting_cluster, VFAT_DIRECTORY_CACHE_DATA *data, void (*callback)(FATFS *fs_ptr, uint8_t status,VFAT_DIRECTORY_CACHE_DATA *data))
{

}

void root_directory_loaded_cb(FATFS *fs_ptr, uint8_t status, void *buffer, void *extradata)
{
  VFAT_DIRECTORY_CACHE_NODE *current_node=NULL, *prev_node=NULL;
  VFAT_DIRECTORY_CACHE_DATA *transient = (VFAT_DIRECTORY_CACHE_DATA *)extradata;

  if(status!=0) {
    printf("ERROR Could not load root directory, error %d\n", status);
    return;
  }
  printf("Loaded root directory at 0x%llx\n", (uint64_t)buffer);

  uint32_t directory_count=0;
  DirectoryEntry *entry;
  // char shortname[14];
  // memset(shortname, 0, 14);
  char attrs[12];
  // memset(attrs, 0, 12);

  while(1) {
      entry = (DirectoryEntry *)(buffer + directory_count*sizeof(DirectoryEntry));
      if(entry->short_name[0]==0) {
        printf("DEBUG Came to the end of the directory after %d entries.\n", directory_count);
        break;
      } else {
        decode_attributes(entry->attributes, attrs);

        if(! (entry->attributes & VFAT_ATTR_LFNCHUNK)) { //we are not an LFN
          current_node = vfat_directory_cache_new_node(fs_ptr->directory_cache);
          if(prev_node) prev_node->next = current_node;
          memset(current_node, 0, sizeof(VFAT_DIRECTORY_CACHE_NODE));

          memcpy(current_node->full_file_name, entry->short_name, 8);
          current_node->full_file_name[9]='.';
          memcpy(&(current_node->full_file_name)[10], entry->short_xtn, 3);
          memcpy(&current_node->entry, entry, sizeof(DirectoryEntry));
          printf("INFO Got %s, size %d, attributes %s\n", current_node->full_file_name, entry->file_size, attrs);

          if(entry->attributes & VFAT_ATTR_SUBDIR) { //we are a directory
            directory_queue_push(transient->dirs_to_process, entry);
            printf("INFO Pushed %s to process in the next iteration, queue length is %d\n", current_node->full_file_name, transient->dirs_to_process->entry_count);
          }
        }
        ++directory_count;
        prev_node = current_node;
        current_node = NULL;
      }
  }
  free(buffer);
}

void initialise_directory_cache(FATFS *fs_ptr,
   void (*callback)(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root))
{
  printf("Caching directory tables...\n");

  void *buffer = malloc(BUFFER_SIZE_IN_PAGES*PAGE_SIZE);

  fs_ptr->directory_cache = (VFAT_DIRECTORY_CACHE *)malloc(PAGE_SIZE);
  memset(fs_ptr->directory_cache, 0, PAGE_SIZE);
  fs_ptr->directory_cache->bytes_allocated = 0;
  fs_ptr->directory_cache->bytes_used = 0;
  fs_ptr->directory_cache->start = NULL;

  //mmmm, not very efficient
  VFAT_DIRECTORY_CACHE_DATA *transient = (VFAT_DIRECTORY_CACHE_DATA *)malloc(PAGE_SIZE);
  memset(transient, 0, PAGE_SIZE);
  transient->dirs_to_process = new_directory_queue(256);  //limit to 256 dirs in one subdir, for the time being.

  if(fs_ptr->f32bpb) {
    uint32_t root_directory_entry_cluster = fs_ptr->f32bpb->root_directory_entry_cluster;
    printf("Found FAT32root directory at cluster %ld\n", root_directory_entry_cluster);
    fat_load_file(fs_ptr, root_directory_entry_cluster, buffer, BUFFER_SIZE_IN_PAGES*PAGE_SIZE, (void *)transient, &root_directory_loaded_cb);
  } else {
    printf("FAT12/FAT16 not implemented yet");
  }
}
