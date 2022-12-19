#include <types.h>
#include <malloc.h>
#include <memops.h>
#include <fs/fat_fileops.h>
#include <fs/fat_dirops.h>
#include <fs/fat_fs.h>
#include <fs/vfat.h>
#include <stdio.h>
#include <errors.h>
#include <string.h>

struct find_8point3_file_transient_data {
  char filename[9];
  char xtn[4];
  void *extradata;
  void (*callback)(uint8_t status, DirectoryEntry *dir_entry, char *extradata);
};

void _vfat_find_8point3_dir_opened(VFatOpenFile* fp, uint8_t status, VFatOpenDir* dir, void* extradata)
{
  struct find_8point3_file_transient_data* transient = (struct find_8point3_file_transient_data *)extradata;
  size_t i=0;

  if(status!=0) {
    if(dir!=NULL) vfat_dir_close(dir);
    if(fp!=NULL) vfat_close(fp);

    transient->callback(status, NULL, transient->extradata);
    free(transient);
    return;
  }

  DirectoryEntry *entry = vfat_read_dir_next(dir);
  size_t filename_len = strlen(transient->filename);
  size_t xtn_len = strlen(transient->xtn);

  while(entry!=NULL) {
    //LFN chunks have a different data format
    if(entry->attributes != VFAT_ATTR_LFNCHUNK && strncmp(entry->short_name, transient->filename, filename_len)==0 && strncmp(entry->short_xtn, transient->xtn, xtn_len)==0) {
      //we found it! Might as well free the directory contents before we fire the callback, but since `entry` is a reference
      //into `dir`'s buffer we must copy it first
      kprintf("Found file at entry %d: %s %s\r\n", i, entry->short_name, entry->short_xtn);
      DirectoryEntry *copied_entry = (DirectoryEntry *)malloc(sizeof(DirectoryEntry));
      memcpy(copied_entry, entry, sizeof(DirectoryEntry));
      vfat_dir_close(dir);
      vfat_close(fp);
      transient->callback(E_OK, copied_entry, extradata);
      free(transient);
      return;
    }
    entry = vfat_read_dir_next(dir);
    i++;
  }
  //we did not find anything :(
  vfat_dir_close(dir);
  vfat_close(fp);
  transient->callback(E_OK, NULL, extradata);
  free(transient);
}

void vfat_find_8point3_in_root_dir(FATFS *fs_ptr, char *filename, char *xtn, void *extradata, void (*callback)(uint8_t status, DirectoryEntry *dir_entry, char *extradata))
{
  struct find_8point3_file_transient_data* transient = (struct find_8point3_file_transient_data *)malloc(sizeof(struct find_8point3_file_transient_data));
  if(!transient) {
    callback(E_NOMEM, NULL, extradata);
    return;
  }

  memset(transient, 0, sizeof(struct find_8point3_file_transient_data));
  strncpy(filename, transient->filename, 8);
  strncpy(xtn, transient->xtn, 3);
  transient->extradata = extradata;
  transient->callback = callback;
  vfat_opendir_root(fs_ptr, (void *)transient, &_vfat_find_8point3_dir_opened);
}

void vfat_decode_attributes(uint8_t attrs, char *buf)
{
  if(attrs&0x0F) {  //Read-only, hidden, system, volume label all set => LFN fragment
    strcpy(buf, "!LFN");
    return;
  }
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

void vfat_get_printable_filename(const DirectoryEntry *entry, char *buf, size_t bufsize)
{
  register size_t i;
  for(i=0;i<8;i++) {
    if(entry->short_name[i]==0x20 || entry->short_name[i]==0) break;
    buf[i] = entry->short_name[i];
  }
  if(entry->short_xtn[0]!=0x20 && entry->short_xtn[0]!=0) {
    i++;
    buf[i] = '.';
    for(register size_t j=0; j<3; j++) {
      if(entry->short_xtn[j]==0x20 || entry->short_xtn[j]==0) break;
      buf[i+j+1] = entry->short_xtn[j];
    }
  }
}

struct opendir_transient_data {
  void* extradata;
  void(*callback)(VFatOpenFile* fp, uint8_t status, VFatOpenDir* dir, void* extradata);
};

DirectoryEntry* vfat_read_dir_next(VFatOpenDir *dir)
{
  size_t byte_offset = dir->current_dir_idx * sizeof(DirectoryEntry);
  if(byte_offset >= dir->length_in_bytes) return NULL;  //we got to the end of the directory

  DirectoryEntry *result = &dir->buffer[dir->current_dir_idx]; //the buffer is typed to DirectoryEntry so we need a block offset not byte offset.
  if(result->file_size==0 && result->attributes==0) return NULL;  //we got to the end of useful content
  ++dir->current_dir_idx;
  return result;
}

void vfat_dir_seek(VFatOpenDir *dir, size_t index)
{
  dir->current_dir_idx = index;
}

void vfat_dir_close(VFatOpenDir *dir)
{
  if(dir->buffer) free(dir->buffer);
  dir->buffer = NULL;
  free(dir);
}

void _vfat_dir_read_completed(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata)
{
  struct opendir_transient_data *t = (struct opendir_transient_data *)extradata;

  if(status!=0) {
    kprintf("ERROR Directory read failed\r\n");
    t->callback(fp, status, NULL, t->extradata);
    free(t);
    if(buf) free(buf);
    return;
  }

  VFatOpenDir *dir = (VFatOpenDir *)malloc(sizeof(VFatOpenDir));
  if(!dir) {
    kprintf("ERROR Could not allocate directory to return\r\n");
    t->callback(fp, status, NULL, t->extradata);
    free(t);
    if(buf) free(buf);
    return;
  }
  /*
  size_t current_dir_idx;
  size_t length_in_bytes;
  size_t length_in_dirs;
  DirectoryEntry* buffer;
  */
  dir->current_dir_idx = 0;
  dir->length_in_bytes = fp->file_length;
  dir->buffer = buf;
  t->callback(fp, 0, dir, t->extradata);
  free(t);
}

void vfat_opendir_root(FATFS *fs_ptr, void* extradata, void(*callback)(VFatOpenFile* fp, uint8_t status, VFatOpenDir* buffer, void* extradata))
{
  size_t start_location_cluster;
  if(fs_ptr->f32bpb) {
    start_location_cluster = fs_ptr->f32bpb->root_directory_entry_cluster;
  } else {
    start_location_cluster = (fs_ptr->bpb->reserved_logical_sectors + (fs_ptr->bpb->fat_count * fs_ptr->bpb->logical_sectors_per_fat)) / fs_ptr->bpb->logical_sectors_per_cluster;
  }

  size_t root_dir_size;
  if(fs_ptr->f32bpb) {
    //we don't actually know the size, but need to impose a limit in order to know how much memory to allocate.
    //Revisit this later.
    root_dir_size = 0xFFFF; //64k to be going on with
  } else {
    root_dir_size = fs_ptr->bpb->max_root_dir_entries * 32;
  }

  VFatOpenFile *fp = vfat_open_by_location(fs_ptr, start_location_cluster, root_dir_size, 0);
  if(!fp) {
    callback(NULL, E_NOMEM, NULL, extradata);
    return;
  }

  void* buffer = malloc(root_dir_size);
  if(!buffer) {
    kprintf("ERROR Could not allocate RAM for root dir\r\n");
    callback(fp, E_NOMEM, NULL, extradata);
    return;
  }
  struct opendir_transient_data *t = (struct opendir_transient_data *)malloc(sizeof(struct opendir_transient_data));
  if(!t) {
    kprintf("ERROR Could not allocate RAM for transient data\r\n");
    callback(fp, E_NOMEM, NULL, extradata);
    return;
  }
  t->extradata = extradata;
  t->callback = callback;

  vfat_read_async(fp, buffer, root_dir_size, (void *)t, &_vfat_dir_read_completed);
}
