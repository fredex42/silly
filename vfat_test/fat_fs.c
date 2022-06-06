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

/**
This callback is invoked when the cluster map has been read in and parsed.
It fills the cluster map into the fs_ptr then signals the final callback to show that the
mount operation has been completed.
*/
void cluster_map_loaded(uint8_t status, struct vfat_cluster_map *map)
{
  FATFS *fs_ptr = map->parent_fs;
  if(map==NULL) {
    printf("ERROR could not load FAT cluster map, status is %d\n", status);
    fs_ptr->busy=0;
    fs_ptr->did_mount_cb(fs_ptr, 2, fs_ptr->mount_data_ptr);
  } else {
    fs_ptr->cluster_map = map;
    printf("Found a FAT%d filesystem\n", map->bitsize);
    fs_ptr->busy=0;
    fs_ptr->did_mount_cb(fs_ptr, status, fs_ptr->mount_data_ptr);
  }
  fs_ptr->mount_data_ptr = NULL;
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

/**
Iterates the given DirectoryContentsList and frees all associated memory
*/
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


void load_cluster_data(FATFS *fs_ptr, uint32_t cluster_num, void *buffer, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
  size_t reserved_sectors = fs_ptr->f32bpb ?
    fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->f32bpb->logical_sectors_per_fat * fs_ptr->bpb->fat_count) :
     fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->bpb->logical_sectors_per_fat * fs_ptr->bpb->fat_count) ;
  size_t sector_of_cluster = (cluster_num-2) * fs_ptr->bpb->logical_sectors_per_cluster;
  size_t byte_offset = (reserved_sectors + sector_of_cluster) * fs_ptr->bpb->bytes_per_logical_sector + fs_ptr->bpb->max_root_dir_entries*32; //add on the root directory length to the "reserved area"

  //size_t cluster_length = fs_ptr->bpb->logical_sectors_per_cluster;

  printf("load_cluster_data: byte_offset is 0x%lx\n", byte_offset);
  fs_ptr->storage->driver_start_read(fs_ptr->drive_nr, sector_of_cluster, fs_ptr->bpb->logical_sectors_per_cluster, buffer, (void *)fs_ptr, callback);
  // lseek(fd, byte_offset, SEEK_SET);
  // read(fd, buffer, cluster_length);
}

/**
This callback is invoked when the header block of the disk has been read.
It fills in the basic disk parameters into fs_ptr and then initiates loading in the
cluster map.
*/
void read_header_block_complete(uint8_t status, void *buffer, void *extradata)
{
  FATFS *fs_ptr = (FATFS *)extradata;
  size_t buffer_loc = 0;

  fs_ptr->start = (BootSectorStart *)buffer;

  buffer_loc += sizeof(BootSectorStart);

  fs_ptr->bpb = (BIOSParameterBlock *)(buffer + buffer_loc);

  buffer_loc += sizeof(BIOSParameterBlock);

  if(fs_ptr->bpb->max_root_dir_entries==0) {
    puts("Probably a FAT32\n");
    //lseek(fd, 0x024, SEEK_SET);
    fs_ptr->f32bpb = (FAT32ExtendedBiosParameterBlock *)(buffer + 0x024);
    fs_ptr->reserved_sectors = fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->f32bpb->logical_sectors_per_fat * fs_ptr->bpb->fat_count);
    //read_fs_information_sector(fd, bpb, f32bpb->fs_information_sector, &infosector);
  } else {
    puts("Probably not FAT32\n");
    fs_ptr->ebpb = (ExtendedBiosParameterBlock *)(buffer + buffer_loc);
    fs_ptr->reserved_sectors = fs_ptr->bpb->reserved_logical_sectors;
  }
  vfat_load_cluster_map(fs_ptr, &cluster_map_loaded);
}

void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata))
{
  char *block_buffer = (char *)malloc(512);
  if(fs_ptr->storage==NULL) {
    printf("fs_ptr->storage is not set!\n");
    callback(fs_ptr, 1, extradata);
  } else {
    fs_ptr->busy = 1;
    fs_ptr->did_mount_cb = callback;
    fs_ptr->drive_nr = drive_nr;
    fs_ptr->mount_data_ptr = extradata;
    fs_ptr->storage->driver_start_read(drive_nr, 0, 1, block_buffer, fs_ptr, &read_header_block_complete);
  }
}

void fat_fs_unmount(struct fat_fs *fs_ptr)
{

}

int first_dir_entry(int fd, BIOSParameterBlock *bpb, uint32_t rootdir_cluster, uint32_t reserved_sectors, DirectoryEntry **entry)
{
  printf("rootdir cluster is %d, sectors per cluster is %d, reserved sectors is %d, bytes per logical sector is %d\n", rootdir_cluster, bpb->logical_sectors_per_cluster, reserved_sectors, bpb->bytes_per_logical_sector);
  off_t offset_in_bytes = (rootdir_cluster * bpb->logical_sectors_per_cluster * bpb->bytes_per_logical_sector) + (reserved_sectors * bpb->bytes_per_logical_sector);

  printf("first_dir_entry offset is 0x%lx\n", offset_in_bytes);
  lseek(fd, offset_in_bytes, SEEK_SET);

  *entry = (DirectoryEntry *) malloc(sizeof(DirectoryEntry));
  read(fd, *entry, sizeof(DirectoryEntry)); //FIXME needs replacing
  return 0;
}

int next_dir_entry(int fd, DirectoryEntry **entry)
{
  *entry = (DirectoryEntry *) malloc(sizeof(DirectoryEntry));
  read(fd, *entry, sizeof(DirectoryEntry)); //FIXME needs replacing
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

/**
Lists out root directory contents
*/
DirectoryContentsList * scan_root_dir(int fd, BIOSParameterBlock *bpb, uint32_t reserved_sectors, uint32_t rootdir_cluster, size_t *list_length)
{
  DirectoryContentsList *result=NULL, *current=NULL;
  size_t i=0;
  char fn[9];
  char xtn[4];

  printf("Size of DirectoryEntry is %ld\n", sizeof(DirectoryEntry));
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
Search the given filesystem for a file with the requested name.
The callback is triggered when the operation completes, either successfully or unsuccessfully
@param fs_ptr - pointer to a struct fat_fs describing the filesystem
@param path - pointer to an ASCIIZ string giving the (full) file path to find.
@param callback - pointer to a function that takes the fs_ptr as the first argument and a pointer to DirectoryEntry as the second argument.
This is invoked when the operation completes, with `entry` NULL if nothing was found or pointing to a DirectoryEntry if it was.
The callback is responsible for freeing the DirectoryEntry memory.
*/
void fat_fs_find_file(struct fat_fs *fs_ptr, char *path, void (*callback)(struct fat_fs *fs_ptr, DirectoryEntry *entry))
{
  char decoded_attrs[12];

  //root_dir_size is filled in by the scan_root_dir function
  size_t root_dir_size=0;

  //FAT32 has a specific location set in the extra bios parameter block. FAT12/16 use a fixed root directory immediately after the FATs.
  uint32_t root_directory_entry_cluster = fs_ptr->f32bpb ?
  fs_ptr->f32bpb->root_directory_entry_cluster-2 :
  (uint32_t)(fs_ptr->bpb->fat_count*fs_ptr->bpb->logical_sectors_per_fat) / (uint32_t)fs_ptr->bpb->logical_sectors_per_cluster;

  DirectoryContentsList *root_dir = scan_root_dir(fs_ptr->drive_nr, fs_ptr->bpb, fs_ptr->reserved_sectors, root_directory_entry_cluster, &root_dir_size);

  printf("Looking for entry called %s...\n", path);

  DirectoryEntry *entry = vfat_recursive_scan(fs_ptr, root_directory_entry_cluster, path, strlen(path));
  callback(fs_ptr, entry);

  // if(entry==NULL) {
  //   puts("Found nothing.\n");
  // } else {
  //   if(entry->attributes&VFAT_ATTR_SUBDIR) {
  //       size_t subdir_size;
  //       puts("Subdirectory contents:\n");
  //       //FIXME: why is our calculation 128 sectors (2 clusters) out?
  //       //because the root directory starts at cluster 2 of the data area which is the same as the end of reserved_sectors?
  //       DirectoryContentsList *contents = scan_root_dir(fs_ptr->drive_nr, fs_ptr->bpb, fs_ptr->reserved_sectors, FAT32_CLUSTER_NUMBER(entry)-2, &subdir_size);
  //       printf("Subdirectory contained %d items\n", subdir_size);
  //       vfat_dispose_directory_contents_list(contents);
  //   } else {
  //       void *buffer = retrieve_file_content(fs_ptr->drive_nr, fs_ptr->bpb, fs_ptr->f32bpb, fs_ptr->cluster_map, entry);
  //       write_buffer(buffer, entry->file_size, "file.out");
  //       free(buffer);
  //   }
  //
  //   printf("Found an entry: ");
  //   vfat_dump_directory_entry(entry);
  // }
}

DirectoryEntry *vfat_recursive_scan(struct fat_fs *fs_ptr, uint32_t rootdir_cluster, char *remaining_path, size_t path_len)
{
  size_t dummy;
  size_t path_split_point=find_in_string(remaining_path, '\\');
  char dir_to_search[12];
  memset(&dir_to_search, 0, 12);
  strncpy((char *)dir_to_search, (const char *)remaining_path, path_split_point > 12 ? 12 : path_split_point);

  printf("vfat_recursive_scan looking for %s with %ld remaining\n", dir_to_search, path_len);
  DirectoryContentsList *l = scan_root_dir(fs_ptr->drive_nr, fs_ptr->bpb, fs_ptr->reserved_sectors, rootdir_cluster, &dummy);
  DirectoryContentsList *subdir = vfat_find_dir_entry(l, dir_to_search);

  if(path_len==0 || path_split_point==-1) {
    DirectoryEntry *result = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
    memcpy(result, subdir->entry, sizeof(DirectoryEntry));
    vfat_dispose_directory_contents_list(l);
    return result;
  } else if(subdir==NULL) {  //we found nothing
    vfat_dispose_directory_contents_list(l);
    return NULL;
  } else {
    DirectoryEntry *result = vfat_recursive_scan(fs_ptr, FAT32_CLUSTER_NUMBER(subdir->entry)-2, &(remaining_path[path_split_point+1]), path_len-path_split_point);
    vfat_dispose_directory_contents_list(l);
    return result;
  }
}

/**
Initialises a new FATFS struct.
Returns NULL if unsuccessful (could not allocate) or the initialised FATFS if successul.
You should call the `mount` function on the returned FATFS
structure to access it.
*/
FATFS* new_fat_fs(uint8_t drive_nr)
{
  FATFS *newstruct = (FATFS *)malloc(sizeof(FATFS));
  if(newstruct==NULL) return NULL;
  memset(newstruct, 0, sizeof(FATFS));

  newstruct->drive_nr = drive_nr;
  newstruct->mount = &fat_fs_mount;
  newstruct->unmount = &fat_fs_unmount;
  newstruct->find_file = &fat_fs_find_file;
  newstruct->busy = 0;
  return newstruct;
}
