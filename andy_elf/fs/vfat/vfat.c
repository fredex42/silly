#include <types.h>
#include "cluster_map.h"
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <sys/mmgr.h>
#include <drivers/generic_storage.h>
#include "../drivers/ata_pio/ata_pio.h"
#include <errors.h>
#include <malloc.h>
#include <memops.h>
#include <stdio.h>
#include <panic.h>
/**
Step four - now the cluster map is loaded, we should be ready to go
*/
void _vfat_loaded_cluster_map(uint8_t status, void *untyped_buffer, void *extradata)
{
  FATFS* new_fs = (FATFS *)extradata;
  uint8_t *buffer = (uint8_t *)untyped_buffer;

  if(status!=0) {
    kprintf("ERROR vfat_mount clustermap load failed\r\n");
    if(buffer) free(buffer);
    //free(new_fs->mount_data_ptr);
    new_fs->mount_data_ptr = NULL;
    new_fs->did_mount_cb(new_fs, status, new_fs->did_mount_cb_extradata);
    return;
  }

  if(buffer[0]<0xF0) {
    kprintf("ERROR Could not recognise FAT type. Header bytes were 0x%x 0x%x 0x%x 0x%x\r\n", (uint32_t)buffer[0], (uint32_t)buffer[1], (uint32_t)buffer[2], (uint32_t)buffer[3]);
    if(buffer) free(buffer);
    new_fs->mount_data_ptr = NULL;
    new_fs->did_mount_cb(new_fs, E_VFAT_NOT_RECOGNIZED, new_fs->did_mount_cb_extradata);
    return;
  }
  //see https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system, under "File allocation table"
  if(buffer[1]==0xFF && buffer[2]==0xFF && buffer[3]!=0xFF) {
    kprintf("INFO Detected a FAT12 filesystem\r\n");
    new_fs->cluster_map->bitsize=12;
  } else if(buffer[1]=0xFF && buffer[2]==0xFF && buffer[3]==0xFF && buffer[4]!=0x0F) {
    kprintf("INFO Detected a FAT16 filesystem\r\n");
    new_fs->cluster_map->bitsize=16;
  } else if(buffer[1]=0xFF && buffer[2]==0xFF && buffer[3]==0x0F) {
    kprintf("INFO Detected a FAT32 filesystem\r\n");
    new_fs->cluster_map->bitsize=32;
  } else {
    kprintf("ERROR Could not recognise FAT type. Header bytes were 0x%x 0x%x 0x%x 0x%x\r\n", (uint32_t)buffer[0], (uint32_t)buffer[1], (uint32_t)buffer[2], (uint32_t)buffer[3]);
    if(buffer) free(buffer);
    new_fs->mount_data_ptr = NULL;
    new_fs->did_mount_cb(new_fs, E_VFAT_NOT_RECOGNIZED, new_fs->did_mount_cb_extradata);
  }
  size_t fat_region_length_sectors = new_fs->bpb->fat_count * (new_fs->f32bpb ? new_fs->f32bpb->logical_sectors_per_fat : new_fs->bpb->logical_sectors_per_fat);
  size_t fat_region_length_bytes   = fat_region_length_sectors * 512; //assume sector size of 512 bytes
  new_fs->did_mount_cb(new_fs, 0, new_fs->did_mount_cb_extradata);
}

/**
Step three - once the basic parameters are loaded, we want to cache the cluster map in memory.
*/
void vfat_load_cluster_map(FATFS* new_fs)
{
  size_t fat_region_length_sectors = new_fs->bpb->fat_count * (new_fs->f32bpb ? new_fs->f32bpb->logical_sectors_per_fat : new_fs->bpb->logical_sectors_per_fat);
  size_t fat_region_length_bytes   = fat_region_length_sectors * 512; //assume sector size of 512 bytes
  kprintf("INFO Cluster map region starts at sector 0x%x and is 0x%x sectors long\r\n", new_fs->bpb->reserved_logical_sectors, fat_region_length_sectors);

  new_fs->cluster_map = (VFatClusterMap *)malloc(sizeof(VFatClusterMap));
  if(!new_fs->cluster_map) {
    new_fs->did_mount_cb(new_fs, E_NOMEM, NULL);
    return;
  }

  new_fs->cluster_map->buffer = (uint8_t*)malloc(fat_region_length_bytes);
  if(!new_fs->cluster_map->buffer) {
    free(new_fs->cluster_map);
    new_fs->did_mount_cb(new_fs, E_NOMEM, NULL);
    return;
  }

  new_fs->cluster_map->buffer_size = fat_region_length_bytes;
  new_fs->cluster_map->parent_fs = new_fs;
  new_fs->cluster_map->bitsize = 0; //we don't know the bit-size yet

  ata_pio_start_read(new_fs->drive_nr, new_fs->bpb->reserved_logical_sectors, fat_region_length_sectors, new_fs->cluster_map->buffer, (void*)new_fs, &_vfat_loaded_cluster_map);
}

/**
Step two - once bootsector is loaded, divvy it up into the right chunks for the FS header
*/
void _vfat_loaded_bootsector(uint8_t status, void *buffer, void *extradata)
{
  FATFS* new_fs = (FATFS *)extradata;

  if(status!=0) {
    kprintf("ERROR vfat_mount bootsector load failed\r\n");
    if(buffer) free(buffer);
    //free(new_fs->mount_data_ptr);
    new_fs->mount_data_ptr = NULL;
    new_fs->did_mount_cb(new_fs, status, new_fs->did_mount_cb_extradata);
    return;
  }

  new_fs->start = (BootSectorStart*) malloc(sizeof(BootSectorStart));
  memcpy(new_fs->start, buffer, sizeof(BootSectorStart));  //first part of the boot sector, unsurprisingly, is BootSectorStart.
  new_fs->bpb = (BIOSParameterBlock*) malloc(sizeof(BIOSParameterBlock));
  memcpy(new_fs->bpb, buffer + 0x0B, sizeof(BIOSParameterBlock));
  if(new_fs->bpb->total_logical_sectors==0 && new_fs->bpb->logical_sectors_per_fat==0) {
    kprintf("INFO vfat filesystem on drive %d is probably FAT32\r\n", new_fs->drive_nr);
    new_fs->f32bpb = (FAT32ExtendedBiosParameterBlock *)malloc(sizeof(FAT32ExtendedBiosParameterBlock));
    memcpy(new_fs->f32bpb, buffer + 0x24, sizeof(FAT32ExtendedBiosParameterBlock));
  } else {
    kprintf("INFO vfat filesystem on drive %d is probably not FAT32\r\n", new_fs->drive_nr);
    new_fs->ebpb = (ExtendedBiosParameterBlock *) malloc(sizeof(ExtendedBiosParameterBlock));
    memcpy(new_fs->ebpb, buffer + 0x24, sizeof(ExtendedBiosParameterBlock));
  }
  free(buffer);

  kprintf("INFO Loading vfat filesystem from drive %d\r\n", new_fs->drive_nr);
  /*
  uint16_t bytes_per_logical_sector;  //Bytes per logical sector; the most common value is 512.
                                      // logical sector size is often identical to a disk's physical sector size, but can be larger or smaller in some scenarios.
  uint8_t logical_sectors_per_cluster;  //Allowed values are 1, 2, 4, 8, 16, 32, 64, and 128
  uint16_t reserved_logical_sectors;    //The number of logical sectors before the first FAT in the file system image.
  uint8_t fat_count;                    //Number of File Allocation Tables. Almost always 2; RAM disks might use 1. Most versions of MS-DOS/PC DOS do not support more than 2 FATs.
  uint16_t max_root_dir_entries;        //Maximum number of FAT12 or FAT16 root directory entries. 0 for FAT32, where the root directory is stored in ordinary data clusters; see offset 0x02C in FAT32 EBPBs.
  uint16_t total_logical_sectors;       //0 for FAT32. (If zero, use 4 byte value at offset 0x020)
  uint8_t media_descriptor;             //see defines above
  uint16_t logical_sectors_per_fat;     //FAT32 sets this to 0 and uses the 32-bit value at offset 0x024 instead.
  */
  kprintf("INFO FS has\r\n    %d bytes per logical sector\r\n    %d logical sectors per cluster\r\n    %d reserved logical sectors\r\n", new_fs->bpb->bytes_per_logical_sector, new_fs->bpb->logical_sectors_per_cluster, new_fs->bpb->reserved_logical_sectors);
  kprintf("    %d FATs\r\n", new_fs->bpb->fat_count);

  if(new_fs->ebpb) {
    kprintf("INFO FS is a FAT12/FAT16 type\r\n");
  }
  if(new_fs->f32bpb) {
    kprintf("INFO FS is a FAT32 type\r\n");
  }
  vfat_load_cluster_map(new_fs);
}

/**
Step one - initialise mount operation
*/
void vfat_mount(FATFS *new_fs, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata))
{
  void *bootsector = malloc(512);
  if(!bootsector) {
    new_fs->did_mount_cb(new_fs, E_NOMEM, new_fs->did_mount_cb_extradata);
    return;
  }

  ata_pio_start_read(drive_nr, 0, 1, bootsector, (void*)new_fs, &_vfat_loaded_bootsector);
}

void vfat_unmount(struct fat_fs *fs_ptr)
{
  kprintf("ERROR vfat_unmount not implemented yet!\r\n");
}

/**
Initialises a new FATFS structure, ready for mounting.  You should only initialise
one per drive_nr.
*/
FATFS* fs_vfat_new(uint8_t drive_nr, void *extradata, void (*did_mount_cb)(struct fat_fs *fs_ptr, uint8_t status, void *extradata))
{
  FATFS* root_fs = (FATFS* )malloc(sizeof(FATFS));
  if(!root_fs) {
    k_panic("Unable to allocate memory to mount root FS\r\n");
  }

  memset(root_fs, 0, sizeof(FATFS));
  root_fs->mount = &vfat_mount;
  root_fs->unmount = &vfat_unmount;
  root_fs->did_mount_cb = did_mount_cb;
  root_fs->did_mount_cb_extradata = extradata;
  root_fs->drive_nr = drive_nr;

  return root_fs;
}
