#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
#include <stdio.h>  //for printf()
#endif

#include "fat_fs.h"
#include "cluster_map.h"
#include "file.h"

//private functions
void load_next_cluster(FILE_LOAD_TRANSIENT_DATA *req, void *buffer, uint32_t loading_cluster);
void _file_cluster_read_cb(uint8_t status, void *buffer, void *fs_ptr, void *extradata);

/**
Called by the IO driver once the given cluster is loaded in. Query the (cached) cluster map
to see what the next cluster to load is, and if so then recurse back to load_next_cluster.
If there is nothing left to load then call the all-done callback.
*/
void _file_cluster_read_cb(uint8_t status, void *buffer, void *fs_ptr, void *extradata)
{
  FILE_LOAD_TRANSIENT_DATA *data = (FILE_LOAD_TRANSIENT_DATA *)extradata;
  uint32_t next_cluster_num = vfat_cluster_map_next_cluster(data->fs_ptr->cluster_map, data->loading_cluster);
  if(next_cluster_num==0xFFFFFFFF) {
    void (*load_done_cb)(FATFS *fs_ptr, uint8_t status, void *buffer) = data->load_done_cb;
    free(data);
    load_done_cb(data->fs_ptr, status, buffer);
  } else {
    load_next_cluster(data, buffer, next_cluster_num);
  }
}

//public functions

/**
Initialises the load of the next cluster of the file
*/
void load_next_cluster(FILE_LOAD_TRANSIENT_DATA *req, void *buffer, uint32_t loading_cluster)
{
  req->loading_cluster = loading_cluster;
  req->fs_ptr->storage->driver_start_read(
    req->fs_ptr->drive_nr,
    SECTOR_FOR_CLUSTER(req->fs_ptr, loading_cluster),
    req->fs_ptr->bpb->logical_sectors_per_cluster,
    (buffer + req->buffer_pos),
    req,
    &_file_cluster_read_cb);
}

void fat_load_file(FATFS *fs_ptr, uint32_t starting_cluster, void *buffer, uint32_t buffer_size, void (*load_done_cb)(FATFS *fs_ptr, uint8_t status, void *buffer))
{
  FILE_LOAD_TRANSIENT_DATA *req = (FILE_LOAD_TRANSIENT_DATA *)malloc(sizeof(FILE_LOAD_TRANSIENT_DATA));
  memset((void *)req, 0, sizeof(FILE_LOAD_TRANSIENT_DATA));

  req->fs_ptr = fs_ptr;
  req->buffer_pos = 0;
  req->loading_cluster = starting_cluster;
  req->buffer_length_in_clusters = buffer_size / BYTES_PER_CLUSTER(fs_ptr);
  req->load_done_cb = load_done_cb;
  load_next_cluster(req, buffer, req->loading_cluster);
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
