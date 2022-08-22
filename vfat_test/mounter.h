#ifndef __MOUNTER_H
#define __MOUNTER_H

void fat_fs_mount(FATFS *fs_ptr, uint8_t drive_nr, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));
#endif
