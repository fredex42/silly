#include <types.h>
#include <fs/fat_fileops.h>
#include <fs/vfat.h>
#ifndef __FS_FAT_DIROPS_H
#define __FS_FAT_DIROPS_H

typedef struct vfat_open_dir {
  size_t current_dir_idx;
  size_t length_in_bytes;
  DirectoryEntry* buffer;
} VFatOpenDir;

void vfat_opendir_root(struct fat_fs *fs_ptr, void* extradata, void(*callback)(VFatOpenFile* fp, uint8_t status, VFatOpenDir* buffer, void* extradata));
void vfat_dir_close(VFatOpenDir *dir);
void vfat_dir_seek(VFatOpenDir *dir, size_t index);
DirectoryEntry* vfat_read_dir_next(VFatOpenDir *dir);

/*
takes a DirectoryEntry and renders a printable version of the (8.3) filename into the buffer
`buf`.  If the buffer is not large enough then the result is truncated
*/
void vfat_get_printable_filename(const DirectoryEntry *entry, char *buf, size_t bufsize);

/*
takes the attributes bitfield from a DirectoryEntry and renders a human-readable string indicating
which attribute bits are set. `buf` must be a pointer to a string buffer 8 bytes long.
The function will null-terminate it.
*/
void vfat_decode_attributes(uint8_t attrs, char *buf);

#endif
