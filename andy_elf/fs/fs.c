#include <types.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <memops.h>
#include <malloc.h>
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

void _fs_root_device_mounted(struct fat_fs *fs_ptr, uint8_t status, void *extradata)
{
  kprintf("INFO Root FS mount completed with status %d\r\n", status);

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
