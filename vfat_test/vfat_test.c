#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fat_fs.h"
#include "ucs_conv.h"
#include "vfat.h"
#include "cluster_map.h"
#include "fake_storage_driver.h"
#include "file.h"
#include "mounter.h"
#include "dir.h"
#include "file.h"

void vfat_dump_directory_entry(DirectoryEntry *entry)
{
  char decoded_attrs[8];
  char fn[9];
  char xtn[4];

  decode_attributes(entry->attributes, (char *)&decoded_attrs);
  strncpy((char *)&fn, entry->short_name, 8);
  strncpy((char *)&xtn, entry->short_xtn, 3);

  if(fn[0]==0xE5) {
    fn[0] = entry->first_char_of_deleted;
    printf("Got DELETED file %s.%s with size %u and attributes 0x%x (%s) at cluster 0x%x\n", fn, xtn, entry->file_size, entry->attributes, decoded_attrs, FAT32_CLUSTER_NUMBER(entry));
  } else {
    printf("Got file %s.%s with size %u and attributes 0x%x (%s) at cluster 0x%x\n", fn, xtn, entry->file_size,entry->attributes, decoded_attrs, FAT32_CLUSTER_NUMBER(entry));
  }
}

// void *retrieve_file_content(int fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, VFatClusterMap *m, DirectoryEntry *entry)
// {
//   uint32_t next_cluster_num;
//   size_t ctr;
//   char *buf = (char *)malloc(entry->file_size + bpb->logical_sectors_per_cluster*bpb->bytes_per_logical_sector);
//   if(buf==NULL) return NULL;
//   printf("retrieve_file_content: buf is 0x%x\n", buf);
//
//   next_cluster_num = FAT32_CLUSTER_NUMBER(entry) & 0x0FFFFFFF;
//   ctr = 0;
//   do {
//       load_cluster_data(fd, bpb, f32bpb, next_cluster_num, &buf[ctr]);
//       ctr += bpb->logical_sectors_per_cluster*bpb->bytes_per_logical_sector;
//       next_cluster_num = vfat_cluster_map_next_cluster(m, next_cluster_num);
//   } while(next_cluster_num<0x0FFFFFFF);
//
//   return (void *)buf;
// }

#define WRITE_BLOCK_SIZE  512

void write_buffer(void *buffer, size_t length, char *filename)
{
  int out = open(filename, O_CREAT|O_WRONLY|O_TRUNC);
  if(out==-1) {
    printf("ERROR Could not open %s for writing\n", filename);
    return;
  }

  for(size_t ctr=0; ctr<length; ctr+=WRITE_BLOCK_SIZE) {
    size_t size_to_write = ctr+WRITE_BLOCK_SIZE>length ? length-ctr : WRITE_BLOCK_SIZE;
    write(out, &(((char *)buffer)[ctr]), size_to_write);
  }
  close(out);
}

void file_found(struct fat_fs *fs_ptr, DirectoryEntry *entry)
{
  char filename[16];
  char attrs[8];
  memset(attrs, 0, 8);

  if(entry==NULL) {
    printf("File was not found\n");
  } else {
    vfat_get_printable_filename(entry, filename);
    decode_attributes(entry->attributes, attrs);
    printf("Found file %s with attributes %s and size %d\n", filename, attrs, entry->file_size);
  }
}

struct find_file_data {
  char *full_path;
  char current_search[255];
  size_t path_point;
  size_t path_length;
  size_t steps;
  uint8_t completed;
  DirectoryEntry *current_directory_entry;
  void (*callback)(struct fat_fs *fs_ptr, DirectoryEntry *d);
};

struct find_file_data* new_find_file_data(char *full_path, void (*callback)(struct fat_fs *fs_ptr, DirectoryEntry *d))
{
  struct find_file_data *d=(struct find_file_data *)malloc(sizeof(struct find_file_data));
  d->full_path = (char *)malloc(strlen(full_path));
  strcpy(d->full_path, full_path);
  d->path_point = 0;
  d->path_length = strlen(full_path);
  memset(d->current_search, 0, 255);
  d->steps = 0;
  d->completed = 0;
  d->callback = callback;
}

void update_to_next_path_part(struct find_file_data *d)
{
  size_t next_path_point=d->path_point;
  while(next_path_point<d->path_length) {
    next_path_point++;
    if(d->full_path[next_path_point]=='/' || d->full_path[next_path_point]=='\\') break;
  }
  if(next_path_point>=d->path_length-1) {
    d->completed = 1;
    d->current_search[0] = 0;
    return;
  }
  strncpy(d->current_search, &(d->full_path[d->path_point]), next_path_point-d->path_point);
  d->path_point = next_path_point+1;
}

/*
This callback is invoked once when the FAT_FS structure is loaded.
*/
void fat_fs_ready(struct fat_fs *fs_ptr, uint8_t status, void *extradata)
{
  char *some_path = (char *)extradata;
  if(status==0) {
    printf("File system on drive %d is ready at 0x%lx\n", fs_ptr->drive_nr, fs_ptr);
    if(some_path !=NULL) {
      //fat_fs_find_file(fs_ptr, some_path, &file_found);
      struct find_file_data *finder = new_find_file_data(some_path, &file_found);
      recursively_find_file(fs_ptr, 0, finder);
      free(finder);
    } else {
      printf("No filename passed to look for.\n");
    }
  } else {
    printf("Could not mount file system from drive %d, status was %d\n", fs_ptr->drive_nr, status);
  }
}



void directory_loaded_cb(FATFS *fs_ptr, uint8_t status, void *buffer, void *extradata)
{
  struct find_file_data *d = (struct find_file_data *)extradata;
  printf("INFO Loaded directory at %s\n", d->current_search);
  update_to_next_path_part(d);
  if(d->completed) {
    printf("INFO Found the file!\n");
    d->callback(fs_ptr, d->current_directory_entry);
    return;
  }

  //include terminating nulls!
  char name[9];
  char xtn[4];
  memset(name, 0, 8);
  memset(xtn, 0, 3);
  split_file_name(d->current_search, name, xtn);

  printf("INFO Next directory portion is %s, name '%s' xtn '%s'\n", d->current_search, name, xtn);
  d->current_directory_entry = vfat_find_in_directory(name, xtn, buffer, 1024);
  if(d->current_directory_entry==NULL) {
    printf("INFO Can't find %s at level %d\n", d->current_search, d->steps);
    d->callback(fs_ptr, NULL);
    return;
  }

  ++d->steps;
  size_t next_cluster_location = d->current_directory_entry->f32_high_cluster_num << 4 | d->current_directory_entry->low_cluster_num;

  //FIXME: we should free the buffer here, but we need to copy d->current_directory_entry rather than reference it for that
  //free(buffer);
  recursively_find_file(fs_ptr, next_cluster_location, d);
}

void recursively_find_file(struct fat_fs *fs_ptr, size_t cluster_location_opt, struct find_file_data *d)
{
  //FIXME: this is 1 cluster, shouldn't be a fixed size.
  //Should actually get the directory size and not guess it.
  char *buf = (char *) malloc(32768);
  size_t cluster_location;
  if(cluster_location_opt==0) {
    cluster_location = vfat_root_directory_cluster_location(fs_ptr);
  } else {
    cluster_location = cluster_location_opt;
  }
  printf("INFO Loading directory at '%s' from cluster location 0x%x\n", d->current_search, cluster_location);
  fat_load_file(fs_ptr, cluster_location, buf, 32768, (void *)d, &directory_loaded_cb);
}

int main(int argc, char *argv[]) {

  uint32_t reserved_sectors;

  VFatClusterMap *cluster_map = NULL;

  if(argc<2) {
    puts("You must specify a disk image to read\n");
    exit(2);
  }

  FakeStorageDriver *drv = new_fake_storage_driver(argv[1]);
  if(drv==NULL) {
    puts("ERROR Could not initialise fake driver\n");
    exit(1);
  }

  FATFS* fs_ptr = new_fat_fs(drv->fd);
  fs_ptr->storage = drv;

  //void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));
  fat_fs_mount(fs_ptr, drv->fd, argv[2], &fat_fs_ready);

  // fs_ptr->mount(fs_ptr, drv->fd, argv[2], &fat_fs_ready);
  //

}
