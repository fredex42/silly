#include <types.h>
#include <fs/fat_fileops.h>
#include <fs/vfat.h>
#ifndef __FS_FAT_DIROPS_H
#define __FS_FAT_DIROPS_H

typedef struct vfat_open_dir {
  size_t current_dir_idx;
  size_t length_in_bytes;
  size_t length_in_dirs;
  DirectoryEntry* buffer;
} VFatOpenDir;

void vfat_opendir_root(struct fat_fs *fs_ptr, void* extradata, void(*callback)(VFatOpenFile* fp, uint8_t status, VFatOpenDir* buffer, void* extradata));
void vfat_dir_close(VFatOpenDir *dir);
void vfat_dir_seek(VFatOpenDir *dir, size_t index);
DirectoryEntry* vfat_read_dir_next(VFatOpenDir *dir);
void vfat_get_printable_filename(const DirectoryEntry *entry, char *buf, size_t bufsize);

#endif
