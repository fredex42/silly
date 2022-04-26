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




int read_fs_information_sector(int fd, BIOSParameterBlock *bpb, uint16_t sector_offset, FSInformationSector **sec)
{
  if(sector_offset==0xFFFF || sector_offset==0) {
    puts("FS Information Sector not supported\n");
    return 0;
  }

  lseek(fd, sector_offset*bpb->bytes_per_logical_sector, SEEK_SET);
  *sec = (FSInformationSector *) malloc(sizeof(FSInformationSector));
  read(fd, *sec, sizeof(FSInformationSector));
  return 0;
}


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

void *retrieve_file_content(int fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, VFatClusterMap *m, DirectoryEntry *entry)
{
  uint32_t next_cluster_num;
  size_t ctr;
  char *buf = (char *)malloc(entry->file_size + bpb->logical_sectors_per_cluster*bpb->bytes_per_logical_sector);
  if(buf==NULL) return NULL;
  printf("retrieve_file_content: buf is 0x%x\n", buf);

  next_cluster_num = FAT32_CLUSTER_NUMBER(entry) & 0x0FFFFFFF;
  ctr = 0;
  do {
      load_cluster_data(fd, bpb, f32bpb, next_cluster_num, &buf[ctr]);
      ctr += bpb->logical_sectors_per_cluster*bpb->bytes_per_logical_sector;
      next_cluster_num = vfat_cluster_map_next_cluster(m, next_cluster_num);
  } while(next_cluster_num<0x0FFFFFFF);

  return (void *)buf;
}

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


/*
This callback is invoked once when the FAT_FS structure is loaded.
*/
void fat_fs_ready(struct fat_fs *fs_ptr, uint8_t status)
{
  if(status==0) {
    printf("File system on drive %d is ready at 0x%lx\n", fs_ptr->drive_nr, fs_ptr);
  } else {
    printf("Could not mount file system from drive %d, status was %d\n", fs_ptr->drive_nr, status);
  }
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
  fs_ptr->mount(fs_ptr, drv->fd, &fat_fs_ready);


}
