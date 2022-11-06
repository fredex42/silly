#include <types.h>
#include "vfat.h"
#include <sys/mmgr.h>
#include <drivers/generic_storage.h>
#include <errors.h>
#include <memops.h>

/*
Loads in the bootsector of the given drive, allocating a 4k page of RAM to put it on.
Returns a pointer to the start of the 4k buffer. Remaining space in the 4k buffer can
be used for other content.
Arguments: drive_nr - drive number for the storage driver
           drv      - pointer to a GenericStorageDriver structure giving the storage layer endpoints
           callback - pointer to a function that is called with the storage layer status and a pointer to the 4k buffer
Returns: byte pointer to the buffer.
*/
int8_t load_bootsector(uint8_t drive_nr, const GenericStorageDriver *drv, void (*callback)(int8_t status, void *buffer))
{
  void *buf = vm_alloc_pages(NULL, 1, MP_READWRITE);
  if(buf==NULL) {
    kputs("ERROR can't allocate memory for bootsector\r\n");
    return -E_NOMEM;
  }

  memset(buf, 0, PAGE_SIZE);

  //read just the first 4 sectors and return the buffer for it. Fire the callback when it's ready.
  //That should occupy 2k of the buffer.
  return drv->driver_start_read(drive_nr, 0, 4, buf, NULL, callback);
}

void interrogate_fat_filesystem(uint8_t status, void *buffer)
{
  char oem_name[9];
  oem_name[9] = 0;

  //FIXME: should validate status

  //These parts are common to all versions
  BootSectorStart *start = (BootSectorStart *)buffer;
  BIOSParameterBlock *base_bpb = (BIOSParameterBlock *)((vaddr)buffer + sizeof(BootSectorStart));
  ExtendedBiosParameterBlock *extended_bpb = (ExtendedBiosParameterBlock *)((vaddr)base_bpb + sizeof(BIOSParameterBlock));

  //copy to local stack so it's null-terminated
  memcpy(oem_name, start->oem_name, 8);

  kprintf("Volume in drive has OEM name %s, bytes per sector is %d, sectors per cluster is %d\r\n",
   oem_name, base_bpb->bytes_per_logical_sector, (uint16_t)base_bpb->logical_sectors_per_cluster);

  kprintf("Reserved sector count %d, FAT count %d, root directory limit %d\r\n",
    base_bpb->reserved_logical_sectors,
    (uint16_t)base_bpb->fat_count,
    base_bpb->max_root_dir_entries
  );

  kprintf("Total logical sectors %d\r\n", base_bpb->total_logical_sectors);

  if(base_bpb->max_root_dir_entries) {
    kputs("We have a FAT32 filesystem\r\n");
    FAT32ExtendedBiosParameterBlock *f32 = (FAT32ExtendedBiosParameterBlock *)((vaddr)base_bpb + sizeof(BIOSParameterBlock));
    kprintf("FAT32 FS Information Sector is at sector #%d\r\n", f32->fs_information_sector);

  }
}

/*
Callback that gets fired once the driver has loaded in the FAT portion of the disk
*/
void vfat_load_buffer_filled(uint8_t status, void *buffer)
{

}

/*
Starts the process of loading the file-allocation table into memory.
Arguments:
 - b - pointer to an initialised FATBuffer.
*/
int8_t start_load_fat(FATBuffer *b, BIOSParameterBlock *bpb, uint8_t drive_nr, const GenericStorageDriver *drv, void (*callback)(uint8_t status, void *data))
{
  //load in enough to fill the buffer.
  uint32_t block_count = b->initial_length_bytes / (uint32_t)bpb->bytes_per_logical_sector;
  kprintf("DEBUG start_load_fat loading %l blocks for %l bytes\r\n", block_count, b->initial_length_bytes);
  uint32_t block_nr = bpb->reserved_logical_sectors; //FAT starts after the reserved area.

  return drv->driver_start_read(drive_nr, block_nr, (uint8_t)block_count, b->buffer, NULL, &vfat_load_buffer_filled);
}

void test_vfat()
{
  int8_t error = load_bootsector(1, ata_pio_get_driver(), &interrogate_fat_filesystem);
  if(error!=E_OK) {
    kprintf("ERROR: Could not load bootsector of drive 1: code %d\r\n", (int16_t)error);
  }
}
