#include <types.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <fs/fat_dirops.h>
#include <memops.h>
#include <malloc.h>
#include <stdio.h>
#include "../drivers/ata_pio/ata_pio.h"
#include <fs.h>
#include <errors.h>
#include "../mmgr/process.h"
#include "../process/elfloader.h"

//pointer to the filesystem for each device
static struct fat_fs* device_fs_map[MAX_DEVICE];

static const GenericStorageDriver ata_driver = {
  &ata_pio_start_read,
  &ata_pio_start_write,
  &print_drive_info
};

/**
Returns the FS pointer for the given device, or NULL if there is not a mounted FS
*/
const FATFS* fs_for_device(uint8_t device_no) {
  if(device_no > MAX_DEVICE) return NULL;
  return device_fs_map[device_no];
}

void fs_initialise()
{
  memset(device_fs_map, 0, sizeof(struct fat_fs)*MAX_DEVICE);
}


void _fs_root_device_mounted(struct fat_fs *fs_ptr, uint8_t status, void *extradata)
{
  kprintf("INFO Root FS mount completed with status %d\r\n", status);
  device_fs_map[0] = fs_ptr;
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
