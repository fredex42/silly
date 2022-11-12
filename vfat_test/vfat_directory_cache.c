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
    printf("INFO Expanding the directory cache by 1 page. New extent is %d\n",c->bytes_allocated+PAGE_SIZE);
    c->start = (VFAT_DIRECTORY_CACHE_NODE *)realloc(c->start, c->bytes_allocated+PAGE_SIZE);
    if(c->start==NULL) {
      printf("ERROR Oh no! out of memory.\n");
      return NULL;
    }
    c->bytes_allocated+=PAGE_SIZE;
  }

  //gcc does pointer arithmatic in elements, not bytes.
  //because `c->start` is typed as VFAT_DIRECTORY_CACHE_NODE, adding one to `c->start`
  //means that we add one _element_, i.e. 380 bytes
  VFAT_DIRECTORY_CACHE_NODE *newnode = (c->start + c->entries);
  ++c->entries;
  c->bytes_used += sizeof(VFAT_DIRECTORY_CACHE_NODE);
  return newnode;
}

void directory_cache_dump_node(VFAT_DIRECTORY_CACHE_NODE *node, uint16_t level)
{
  for(int i=0; i<4*level; i++) putchar(' ');
  puts(node->full_file_name);
  putchar('\n');
}

void directory_cache_dump(VFAT_DIRECTORY_CACHE *c)
{
  printf("INFO Directory cache uses %d bytes out of %d allocated\n", c->bytes_used, c->bytes_allocated);

  VFAT_DIRECTORY_CACHE_NODE *node = (c->start + 1);
  while(node!=NULL && node->entry.short_name[0]!=0) {
     directory_cache_dump_node(node, 1);
     node += 1;
  }
}
// void directory_cache_dump(VFAT_DIRECTORY_CACHE_NODE *node, uint16_t level)
// {
//   uint16_t i;
//   if(!node) return;
//   for(i=0; i<4*level; i++) putchar(' ');
//   puts(node->full_file_name);
//   putchar('\n');
//   if(node->child) {
//     directory_cache_dump(node->child, level+1);
//   }
//   if(node->next) {
//     directory_cache_dump(node->next, level);
//   }
// }

void root_directory_loaded_cb(FATFS *fs_ptr, uint8_t status, void *buffer, void *extradata)
{
  VFAT_DIRECTORY_CACHE_NODE *current_node=NULL, *prev_node=NULL;
  VFAT_DIRECTORY_CACHE_DATA *transient = (VFAT_DIRECTORY_CACHE_DATA *)extradata;

  if(status!=0) {
    printf("ERROR Could not load directory, error %d\n", status);
    return;
  }
  printf("Loaded directory at 0x%llx\n", (uint64_t)buffer);

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
          current_node->full_file_name[8]='.';
          memcpy(&(current_node->full_file_name)[9], entry->short_xtn, 3);
          memcpy(&current_node->entry, entry, sizeof(DirectoryEntry));
          //printf("INFO Got %s, size %d, attributes %s\n", current_node->full_file_name, entry->file_size, attrs);

          if(entry->attributes & VFAT_ATTR_SUBDIR) { //we are a directory
            if(entry->short_name[0]!='.') { //FIXME: need a more sophisticated comparison to detect '.' and '..' dirs so we don't recurse forever
              directory_queue_push(transient->dirs_to_process, entry);
              printf("INFO Pushed %s to process in the next iteration, queue length is %d\n", current_node->full_file_name, transient->dirs_to_process->entry_count);
            }
          }
        }
        ++directory_count;
        //
        // if(current_node) add_node_to_cache(fs_ptr->directory_cache, current_node);
        // //if(current_node) link_cache_node(fs_ptr->directory_cache, current_node, prev_node, transient->parent);
        prev_node = current_node;
      }
  }

  if(!directory_queue_empty(transient->dirs_to_process)) {
    printf("INFO %d more directories to go\n", transient->dirs_to_process->entry_count);
    DirectoryEntry next_ent;
    directory_queue_pop(transient->dirs_to_process, &next_ent);
    uint32_t fat32_cluster_num = (next_ent.f32_high_cluster_num << 4) + next_ent.low_cluster_num;
    printf("INFO Next directory entry at cluster 0x%x\n", fat32_cluster_num);
    fat_load_file(fs_ptr, fat32_cluster_num, buffer, BUFFER_SIZE_IN_PAGES*PAGE_SIZE, (void *)transient, &root_directory_loaded_cb);
  } else {
    printf("INFO Processed all directories\n");
    //FIXME: why does free(buffer) here abort on an invalid pointer?
    //Most likely, becuase the `buffer` pointer here is not the actual pointer initialised by malloc()
    //file.c loader functions do some pointer arithmatic to load blocks into the overall memory buffer and this function
    //is called on the last iteration, so `buffer` here will only be the last portion to be loaded.
    //free(buffer);
    directory_queue_unref(transient->dirs_to_process);
    transient->dirs_to_process=NULL;
  }
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

    directory_cache_dump(fs_ptr->directory_cache);
    printf("All done!\n");

  } else {
    printf("FAT12/FAT16 not implemented yet");
  }
}
