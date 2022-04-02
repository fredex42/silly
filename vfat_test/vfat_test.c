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

/**
Return the index of the first occurrence of the character c in the string buf.
Returns -1 if the character is not present.
*/
size_t find_in_string(const char *buf, char c)
{
  for(size_t i=0;i<strlen(buf);i++) {
    if(buf[i]==c) return i;
  }
  return -1;
}


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


int first_dir_entry(int fd, BIOSParameterBlock *bpb, uint32_t rootdir_cluster, uint32_t reserved_sectors, DirectoryEntry **entry)
{
  printf("rootdir cluster is %d, sectors per cluster is %d, reserved sectors is %d, bytes per logical sector is %d\n", rootdir_cluster, bpb->logical_sectors_per_cluster, reserved_sectors, bpb->bytes_per_logical_sector);
  off_t offset_in_bytes = (rootdir_cluster * bpb->logical_sectors_per_cluster * bpb->bytes_per_logical_sector) + (reserved_sectors * bpb->bytes_per_logical_sector);

  printf("first_dir_entry offset is 0x%x\n", offset_in_bytes);
  lseek(fd, offset_in_bytes, SEEK_SET);

  *entry = (DirectoryEntry *) malloc(sizeof(DirectoryEntry));
  read(fd, *entry, sizeof(DirectoryEntry));
  return 0;
}

int next_dir_entry(int fd, DirectoryEntry **entry)
{
  *entry = (DirectoryEntry *) malloc(sizeof(DirectoryEntry));
  read(fd, *entry, sizeof(DirectoryEntry));
  return 0;
}

void decode_attributes(uint8_t attrs, char *buf)
{
  if(attrs&VFAT_ATTR_READONLY) {
    buf[0]='R';
  } else {
    buf[0]='.';
  }
  if(attrs&VFAT_ATTR_HIDDEN) {
    buf[1]='H';
  } else {
    buf[1]='.';
  }
  if(attrs&VFAT_ATTR_SYSTEM) {
    buf[2]='S';
  } else {
    buf[2]='.';
  }
  if(attrs&VFAT_ATTR_VOLLABEL) {
    buf[3]='L';
  } else {
    buf[3]='.';
  }
  if(attrs&VFAT_ATTR_SUBDIR) {
    buf[4]='D';
  } else {
    buf[4]='.';
  }
  if(attrs&VFAT_ATTR_ARCHIVE) {
    buf[5]='A';
  } else {
    buf[5]='.';
  }
  if(attrs&VFAT_ATTR_DEVICE) {
    buf[6]='D';
  } else {
    buf[6]='.';
  }
  buf[7]=0;
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

DirectoryContentsList * scan_root_dir(int fd, BIOSParameterBlock *bpb, uint32_t reserved_sectors, uint32_t rootdir_cluster, size_t *list_length)
{
  DirectoryContentsList *result=NULL, *current=NULL;
  size_t i=0;
  char fn[9];
  char xtn[4];

  printf("Size of DirectoryEntry is %d\n", sizeof(DirectoryEntry));
  result = (DirectoryContentsList *)malloc(sizeof(DirectoryContentsList));
  current = result;
  first_dir_entry(fd, bpb, rootdir_cluster, reserved_sectors, &(result->entry));

  while(current->entry != NULL && current->entry->short_name[0]!=0) {
    i++;

    if(current->entry->attributes==VFAT_ATTR_LFNCHUNK) {
      //FIXME - don't call unicode_to_ascii like this!!!
      char *chars_one = unicode_to_ascii(current->lfn->chars_one, 10);
      char *chars_two = unicode_to_ascii(current->lfn->chars_two, 12);
      char *chars_three= unicode_to_ascii(current->lfn->chars_three, 4);

      printf("Got LFN sequence 0x%x with %s%s%s\n",current->lfn->sequence, chars_one, chars_two, chars_three);
      if(chars_three) free(chars_three);
      if(chars_two) free(chars_two);
      if(chars_one) free(chars_one);

    } else {
      strncpy((char *)&fn, current->entry->short_name, 8);
      strncpy((char *)&xtn, current->entry->short_xtn, 3);

      size_t fn_trim_point = find_in_string((const char *)fn, 0x20);
      if(fn_trim_point==-1) fn_trim_point=8;
      size_t xtn_trim_point = find_in_string((const char *)xtn, 0x20);
      if(xtn_trim_point==-1) xtn_trim_point=3;

      memset(current->short_name,0,12);
      strncpy((char *)current->short_name, (const char *)fn, fn_trim_point);
      if(xtn_trim_point>0 && xtn_trim_point<4) {
        current->short_name[fn_trim_point] = '.';
        strncpy((char *)(&current->short_name[fn_trim_point+1]), xtn, xtn_trim_point);
      }
      vfat_dump_directory_entry(current->entry);
    }
    current->next = (DirectoryContentsList *)malloc(sizeof(DirectoryContentsList));
    next_dir_entry(fd, &(current->next->entry));
    current = current->next;
  }
  *list_length=i;
  return result;
}


/**
Scans a DirectoryContentsList to find an entry matching the given name
*/
DirectoryContentsList *vfat_find_dir_entry(DirectoryContentsList *l, const char *unpadded_name)
{
  for(DirectoryContentsList *entry=l;entry!=NULL;entry=entry->next) {
    if(strncmp(entry->short_name, unpadded_name, 12)==0) return entry;
  }
  return NULL;
}

void vfat_dispose_directory_contents_list(DirectoryContentsList *l)
{
  DirectoryContentsList *entry=l;
  do {
    DirectoryContentsList *next = entry->next;
    if(entry->entry) free(entry->entry);
    free(entry);
    entry=next;
  } while(entry);
}


void load_cluster_data(int fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, uint32_t cluster_num, void *buffer)
{
  size_t reserved_sectors = f32bpb ? bpb->reserved_logical_sectors + (f32bpb->logical_sectors_per_fat * bpb->fat_count) : bpb->reserved_logical_sectors + (bpb->logical_sectors_per_fat * bpb->fat_count) ;
  size_t sector_of_cluster = (cluster_num-2) * bpb->logical_sectors_per_cluster;
  size_t byte_offset = (reserved_sectors + sector_of_cluster) * bpb->bytes_per_logical_sector + bpb->max_root_dir_entries*32; //add on the root directory length to the "reserved area"

  size_t cluster_length = bpb->logical_sectors_per_cluster*bpb->bytes_per_logical_sector;

  printf("load_cluster_data: byte_offset is 0x%x\n", byte_offset);
  lseek(fd, byte_offset, SEEK_SET);
  read(fd, buffer, cluster_length);
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
void fat_fs_ready(struct fat_fs *fs_ptr, uint8_t status, uint8_t drive_nr)
{
  if(status==0) {
    printf("File system on drive %d is ready at 0x%x\n", drive_nr, fs_ptr);
  } else {
    printf("Could not mount file system from drive %d, status was %d\n", drive_nr, status);
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
