#ifdef __BUILDING_TESTS
#include <stdint.h>
#include <sys/types.h>
#include <string.h> //for strncmp()
#include <stdio.h>  //for printf()
#endif

#include "vfat.h"
#include "fat_fs.h"

const DirectoryEntry* vfat_peek_next_directory_entry(void *buffer, size_t buffer_size, size_t index)
{
  if(index*sizeof(DirectoryEntry) > buffer_size) return NULL;
  return ((const DirectoryEntry *)buffer + index);
}

/*
Searches for the 8.3 filename in a given directory.
Note that `name` is expected to be all-caps and 8 chars long; `xtn` is expected to be
all-caps and 3 chars long. Padding is with 0x20 (space) chars.
@param name - 8 char name to find
@param xtn - 3 char file extension to find
@param buffer - untyped buffer containing the directory data
@param buffer_size - size of the buffer.
*/
const DirectoryEntry* vfat_find_in_directory(char *name, char *xtn, void *buffer, size_t buffer_size)
{
  size_t i = 0;
  while(1) {
    const DirectoryEntry* e = vfat_peek_next_directory_entry(buffer, buffer_size, i);
    if(e==NULL) return NULL;
    if(strncmp(e->short_name, name, 8)==0 && strncmp(e->short_xtn, xtn, 3)==0) return e;
    i++;
  }
}

size_t vfat_root_directory_cluster_location(FATFS *fs_ptr)
{
  return fs_ptr->f32bpb ? fs_ptr->f32bpb->root_directory_entry_cluster : (fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->bpb->logical_sectors_per_fat*fs_ptr->bpb->fat_count))*fs_ptr->bpb->logical_sectors_per_cluster;
}

void vfat_get_printable_filename(const DirectoryEntry *entry, char *buf, size_t bufsize)
{
  register size_t i;
  for(i=0;i<8;i++) {
    if(entry->short_name[i]==0x20 || entry->short_name[i]==0) break;
    buf[i] = entry->short_name[i];
  }
  if(entry->short_xtn[0]!=0x20 && entry->short_xtn[0]!=0) {
    i++;
    buf[i] = '.';
    for(register size_t j=0; j<3; j++) {
      if(entry->short_xtn[j]==0x20 || entry->short_xtn[j]==0) break;
      buf[i+j+1] = entry->short_xtn[j];
    }
  }
}

void split_file_name(char *name, char *stem, char *xtn)
{
  size_t first_dot=0;
  size_t name_length = strlen(name);
  for(first_dot=0;first_dot<name_length;first_dot++) {
    if(name[first_dot]=='.') break;
  }

  strncpy(stem, name, first_dot);
  size_t pad_from = strlen(name);
  for(size_t i=pad_from; i<8; i++) stem[i] = ' ';
  stem[8] = 0;

  if(first_dot+1<name_length) {
    strncpy(xtn, &name[first_dot+1], name_length-first_dot-1);
  } else {
    xtn[0] = 0;
  }

  pad_from = strlen(xtn);
  for(size_t i=pad_from; i<3; i++) xtn[i] = ' ';
  xtn[3] = 0;
}
