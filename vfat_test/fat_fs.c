#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
//#include <unistd.h> //for lseek()
#endif

#include "generic_storage.h"
#include "fat_fs.h"


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
}

void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void (*callback)(uint8_t drive_nr, struct fat_fs *fs_ptr))
{
  char *block_buffer = (char *)malloc(512);
  fs_ptr->storage->driver_start_read(drive_nr, 0, 1, block_buffer, fs_ptr, &read_header_block_complete);
}

void fat_fs_unmount(struct fat_fs *fs_ptr)
{

}

void fat_fs_find_file(struct fat_fs *fs_ptr, char *path, void (*callback)(struct fat_fs *fs_ptr))
{

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
