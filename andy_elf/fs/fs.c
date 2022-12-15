#include <types.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <fs/fat_dirops.h>
#include <memops.h>
#include <malloc.h>
#include <stdio.h>
#include "../drivers/ata_pio/ata_pio.h"
#include "fs.h"

//pointer to the filesystem for each device
static struct fat_fs* device_fs_map[MAX_DEVICE];

static const GenericStorageDriver ata_driver = {
  &ata_pio_start_read,
  &ata_pio_start_write,
  &print_drive_info
};

void fs_initialise()
{
  memset(device_fs_map, 0, sizeof(struct fat_fs)*MAX_DEVICE);
}

void _fs_root_dir_opened(VFatOpenFile* fp, uint8_t status, VFatOpenDir* dir, void* extradata)
{
  char filename_buffer[32];
  char attrs[16];

  memset(filename_buffer, 0, 32);
  memset(attrs, 0, 16);

  if(status!=0) {
    kprintf("ERROR Could not open root directory: error %d\r\n", status);
    vfat_close(fp);
    return;
  }
  kprintf("INFO Root directory contents:\r\n");
  DirectoryEntry *entry = vfat_read_dir_next(dir);
  while(entry!=NULL) {
    if(entry->attributes != VFAT_ATTR_LFNCHUNK) {
      vfat_get_printable_filename(entry, filename_buffer, 32);
      vfat_decode_attributes(entry->attributes, attrs);
      kprintf("DEBUG Found file %s %s size %l\r\n", entry, attrs, entry->file_size);
    }
    entry = vfat_read_dir_next(dir);
  }
  kprintf("INFO Root directory contents list completed\r\n");
  vfat_dir_close(dir);
  vfat_close(fp);
}

void _fs_root_device_mounted(struct fat_fs *fs_ptr, uint8_t status, void *extradata)
{
  kprintf("INFO Root FS mount completed with status %d\r\n", status);
  device_fs_map[0] = fs_ptr;

  vfat_opendir_root(fs_ptr, NULL, &_fs_root_dir_opened);
}

void mount_root_device()
{
  kprintf("INFO Initialising FS of device 0\r\n");
  FATFS* root_fs = fs_vfat_new(0, NULL, &_fs_root_device_mounted);
  if(!root_fs) {
    k_panic("Unable to intialise root filesystem descriptor\r\n");
  }
  device_fs_map[0] = root_fs;
  kprintf("INFO Starting mount of device 0\r\n");
  root_fs->mount(root_fs, 0, NULL, &_fs_root_device_mounted);
}
