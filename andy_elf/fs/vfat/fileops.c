#include <types.h>
#include <malloc.h>
#include <memops.h>
#include "../drivers/ata_pio/ata_pio.h"
#include <fs/fat_fs.h>
#include <fs/fat_fileops.h>
#include <stdio.h>
#include <panic.h>
#include "cluster_map.h"

/**
Cluster 2 in the directory tables and FAT actually refers to the start of the data area.
This means that we must apply a sector offset in order to translate it to an actual location
on the disk (otherwise we effectively discount the disk headers by assuming that the data region starts at 0)
*/
size_t vfat_get_sector_offset(FATFS *fs_ptr) {
  if(fs_ptr->f32bpb) {
    size_t root_dir_location_cluster = fs_ptr->f32bpb->root_directory_entry_cluster;
    k_panic("vfat_add_fs_offset not implemented yet for FAT32\r\n");
  } else {
    size_t root_dir_location_sector = (fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->bpb->fat_count * fs_ptr->bpb->logical_sectors_per_fat));
    size_t root_dir_size_sectors = (fs_ptr->bpb->max_root_dir_entries * sizeof(DirectoryEntry)) / ATA_SECTOR_SIZE;
    size_t sector_offset = (root_dir_location_sector + root_dir_size_sectors) - 2*fs_ptr->bpb->logical_sectors_per_cluster; //cluster 2 is the start of the data area
    return sector_offset;
  }
}


/**
Low-level open function. This is used on files and directories.
*/
VFatOpenFile* vfat_open_by_location(FATFS *fs_ptr, size_t cluster_location_start, size_t file_size, size_t sector_offset)
{
  VFatOpenFile *fp = (VFatOpenFile *)malloc(sizeof(VFatOpenFile));
  if(!fp) {
    kprintf("ERROR Insufficient memory to alloc another open file\r\n");
    return NULL;
  }

  fp->parent_fs = fs_ptr;
  fs_ptr->open_file_count++;
  fp->current_cluster_number = cluster_location_start;
  fp->sector_offset_in_cluster = 0;
  fp->byte_offset_in_cluster = 0;
  fp->file_length = file_size;
  fp->fs_sector_offset = sector_offset;
  return fp;
}

VFatOpenFile* vfat_open(FATFS *fs_ptr, DirectoryEntry* entry_to_open)
{
  size_t cluster_number = entry_to_open->low_cluster_num;
  if(fs_ptr->f32bpb) cluster_number += ((uint32_t)entry_to_open->f32_high_cluster_num << 16);
  kprintf("DEBUG vfat_open cluster_number is 0x%x (32-bit), size is 0x%x\r\n", cluster_number, entry_to_open->file_size);

  return vfat_open_by_location(fs_ptr, cluster_number, entry_to_open->file_size, vfat_get_sector_offset(fs_ptr));
}


void vfat_close(VFatOpenFile *fp)
{
  fp->parent_fs->open_file_count--;
  free(fp);
}

struct vfat_read_transient_data {
  void(*callback)(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata);
  void* cb_extradata;
  void* real_buffer;
  VFatOpenFile* fp;
  size_t requested_length;
  size_t buffer_write_offset;
  size_t disk_read_offset;
};

void _vfat_next_block_read(uint8_t status, void *buffer, void *extradata)
{
  struct vfat_read_transient_data* t = (struct vfat_read_transient_data *)extradata;

  if(status!=0) {
    kprintf("ERROR reading from file 0x%x at offset 0x%d.\r\n", t->fp, t->buffer_write_offset);
    if(buffer) free(buffer);
    t->fp->busy = 0;
    t->callback(t->fp, status, t->buffer_write_offset, t->real_buffer, t->cb_extradata);
    free(t);
    return;
  }

  size_t bytes_to_copy =  512;
  //OK, so we have 1 sector (512 bytes) back. Let's copy it into the real buffer now.
  if(t->disk_read_offset==0 && t->buffer_write_offset+512 < t->requested_length) {
    memcpy_dw(t->real_buffer + t->buffer_write_offset, buffer, 64); //copy 64 32-bit DWORDS
    t->buffer_write_offset += 512;
  } else {
    //quick GOOJ: if we are not doing a whole block then copy by byte instead.
    bytes_to_copy = 512 - t->disk_read_offset;
    if(bytes_to_copy +  t->buffer_write_offset > t->requested_length) bytes_to_copy = t->requested_length - t->buffer_write_offset;
    kprintf("DEBUG Copying 0x%x bytes to buffer, from offset 0x%x\r\n", bytes_to_copy, t->buffer_write_offset);
    memcpy(t->real_buffer + t->buffer_write_offset, buffer + t->disk_read_offset, bytes_to_copy);
    t->buffer_write_offset += bytes_to_copy;
  }

  VFatOpenFile *fp = t->fp;

  if(bytes_to_copy>=t->requested_length) {
    //we are done!
    free(buffer);
    kprintf("DEBUG finishing read because output buffer is full.\r\n");
    t->fp->busy = 0;
    t->callback(t->fp, status, t->buffer_write_offset, t->real_buffer, t->cb_extradata);
    free(t);
    return;
  } else {
    //read in the next sector
    ++fp->sector_offset_in_cluster;
    if(fp->sector_offset_in_cluster >= fp->parent_fs->bpb->logical_sectors_per_cluster) {
      fp->sector_offset_in_cluster = 0;
      fp->current_cluster_number = vfat_cluster_map_next_cluster(fp->parent_fs->cluster_map, fp->current_cluster_number);
    }
    kprintf("DEBUG at sector offset %d in cluster 0x%x\r\n", fp->sector_offset_in_cluster, fp->current_cluster_number);
    if(fp->current_cluster_number==CLUSTER_MAP_EOF_MARKER) {
      //we are done!
      free(buffer);
      t->fp->busy = 0;
      t->callback(t->fp, status, t->buffer_write_offset, t->real_buffer, t->cb_extradata);
      free(t);
      return;
    } else {
      uint64_t next_sector_number =  (fp->current_cluster_number * fp->parent_fs->bpb->logical_sectors_per_cluster) + fp->sector_offset_in_cluster + fp->fs_sector_offset;
      kprintf("DEBUG Current cluster number os 0x%x, sectors per cluster is %d, next sector number is 0x%x\r\n", fp->current_cluster_number, fp->parent_fs->bpb->logical_sectors_per_cluster, (uint32_t)next_sector_number);
      //FIXME need to check return value for E_BUSY and reschedule if so
      ata_pio_start_read(fp->parent_fs->drive_nr, next_sector_number, 1, buffer, (void *)t, &_vfat_next_block_read);
    }
  }
}

/**
Reads bytes (well, sectors) from the given open file into the given buffer.
We attempt to read `length` bytes from the current (sector) position in the file.
*/
void vfat_read_async(VFatOpenFile *fp, void* buf, size_t length, void* extradata, void(*callback)(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata))
{
  if(fp->busy) {
    callback(fp, ERR_FS_BUSY, 0, NULL, extradata);
    return;
  }

  fp->busy = 1;
  struct vfat_read_transient_data *t = (struct vfat_read_transient_data *)malloc(sizeof(struct vfat_read_transient_data));
  if(!t) {
    kprintf("ERROR Could not allocate space for vfat transient data\r\n");
    return;
  }
  t->callback = callback;
  t->cb_extradata = extradata;
  t->real_buffer = buf;
  t->fp = fp;
  t->requested_length = length;
  t->buffer_write_offset = 0;
  t->disk_read_offset = 0;  //FIXME we should be able to read from arbitary location

  void* sector_buffer = malloc(ATA_SECTOR_SIZE);

  uint64_t initial_sector = (fp->current_cluster_number * fp->parent_fs->bpb->logical_sectors_per_cluster) + fp->sector_offset_in_cluster + fp->fs_sector_offset;

  ata_pio_start_read(fp->parent_fs->drive_nr, initial_sector, 1, sector_buffer, (void*) t, &_vfat_next_block_read);
}
