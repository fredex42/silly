#include <types.h>
#include <malloc.h>
#include <memops.h>
#include <fs/fat_fileops.h>
#include <fs/fat_dirops.h>
#include <fs/fat_fs.h>

struct opendir_transient_data {
  void* extradata;
  void(*callback)(VFatOpenFile* fp, uint8_t status, VFatOpenDir* dir, void* extradata);
};

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

DirectoryEntry* vfat_read_dir_next(VFatOpenDir *dir)
{
  size_t byte_offset = dir->current_dir_idx * sizeof(DirectoryEntry);
  if(byte_offset >= dir->length_in_bytes) return NULL;  //we got to the end of the directory
  return &dir->buffer[dir->current_dir_idx]; //the buffer is typed to DirectoryEntry so we need a block offset not byte offset.
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
  kprintf("DEBUG Directory read completed with status %d\r\n", status);

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
  dir->length_in_dirs = 0;  //FIXME, maybe we don't need this?
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

  kprintf("DEBUG root directory location calculated at 0x%x\r\n", start_location_cluster);
  size_t root_dir_size;
  if(fs_ptr->f32bpb) {
    //we don't actually know the size, but need to impose a limit in order to know how much memory to allocate.
    //Revisit this later.
    root_dir_size = 0xFFFF; //64k to be going on with
  } else {
    root_dir_size = fs_ptr->bpb->max_root_dir_entries * 32;
  }
  kprintf("DEBUG root directory size (limit) calculated at 0x%x\r\n",root_dir_size);

  VFatOpenFile *fp = vfat_open_by_location(fs_ptr, start_location_cluster, root_dir_size);
  if(!fp) {
    kprintf("ERROR Could not open root directory area.\r\n");
    callback(NULL, 1, NULL, extradata); //FIXME: need a proper error code
    return;
  }

  void* buffer = malloc(root_dir_size);
  if(!buffer) {
    kprintf("ERROR Could not allocate RAM for root dir\r\n");
    callback(fp, 1, NULL, extradata); //FIXME: need a proper error code
    return;
  }
  struct opendir_transient_data *t = (struct opendir_transient_data *)malloc(sizeof(struct opendir_transient_data));
  if(!t) {
    kprintf("ERROR Could not allocate RAM for transient data\r\n");
    callback(fp, 1, NULL, extradata); //FIXME: need a proper error code
    return;
  }
  t->extradata = extradata;
  t->callback = callback;

  vfat_read_async(fp, buffer, root_dir_size, (void *)t, &_vfat_dir_read_completed);
}
