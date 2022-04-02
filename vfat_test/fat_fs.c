#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
//#include <unistd.h> //for lseek()
#endif

#include "generic_storage.h"
#include "fat_fs.h"
#include "cluster_map.h"

/**
This callback is invoked when the cluster map has been read in and parsed.
It fills the cluster map into the fs_ptr then signals the final callback to show that the
mount operation has been completed.
*/
void cluster_map_loaded(uint8_t status, FATFS *fs_ptr, struct vfat_cluster_map *map)
{
  fs_ptr->cluster_map = map;
  printf("Found a FAT%d filesystem\n", map->bitsize);

  fs_ptr->did_mount_cb(fs_ptr, status);
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
  //read(fd, *start, sizeof(BootSectorStart));

  //*(ptrs->bpb) = (BIOSParameterBlock *) malloc(sizeof(BIOSParameterBlock));
  //read(fd, *bpb, sizeof(BIOSParameterBlock));
  //memcpy(bpb, buffer+buffer_loc, sizeof(BiosParameterBlock));
  fs_ptr->bpb = (BIOSParameterBlock *)(buffer + buffer_loc);

  buffer_loc += sizeof(BIOSParameterBlock);

  if(fs_ptr->bpb->max_root_dir_entries==0) {
    puts("Probably a FAT32\n");
    //lseek(fd, 0x024, SEEK_SET);
    fs_ptr->f32bpb = (FAT32ExtendedBiosParameterBlock *)(buffer + buffer_loc);
    fs_ptr->reserved_sectors = fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->f32bpb->logical_sectors_per_fat * fs_ptr->bpb->fat_count);
    //read(fd, *f32bpb, sizeof(FAT32ExtendedBiosParameterBlock));
    //read_fs_information_sector(fd, bpb, f32bpb->fs_information_sector, &infosector);
  } else {
    puts("Probably not FAT32\n");
    fs_ptr->ebpb = (ExtendedBiosParameterBlock *)(buffer + buffer_loc);
    fs_ptr->reserved_sectors = fs_ptr->bpb->reserved_logical_sectors;
    //read(fd, *ebpb, sizeof(ExtendedBiosParameterBlock));
  }
  vfat_load_cluster_map(fs_ptr, &cluster_map_loaded);
}

void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void (*callback)(uint8_t drive_nr, struct fat_fs *fs_ptr))
{
  char *block_buffer = (char *)malloc(512);
  fs_ptr->storage->driver_start_read(drive_nr, 0, 1, block_buffer, fs_ptr, &read_header_block_complete);
}

void fat_fs_unmount(struct fat_fs *fs_ptr)
{

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

  DirectoryEntry *entry = vfat_recursive_scan(fs_ptr->drive_nr, fs_ptr->bpb, fs_ptr->reserved_sectors, root_directory_entry_cluster, path, strlen(path));
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


/**
Initialises a new FATFS struct and mounts the given drive number onto it.
Returns -1 if unsuccessful (could not allocate) or 0 if successul.
The given callback function is called once the filesystem is ready.
*/
uint8_t new_fat_fs(uint8_t drive_nr, void (*callback)(uint8_t drive_nr, FATFS *fs_ptr))
{
  FATFS *newstruct = (FATFS *)malloc(sizeof(FATFS));
  if(newstruct==NULL) return -1;
  memset(newstruct, 0, sizeof(FATFS));

  newstruct->mount = &fat_fs_mount;
  newstruct->unmount = &fat_fs_unmount;
  newstruct->find_file = &fat_fs_find_file;

  newstruct->mount(newstruct, drive_nr, callback);
  return 0;
}
