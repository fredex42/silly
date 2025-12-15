#include <types.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <fs/fat_dirops.h>
#include <memops.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include "../drivers/ata_pio/ata_pio.h"
#include <fs.h>
#include <errors.h>
#include <volmgr.h>
#include "../mmgr/process.h"
#include "../process/elfloader.h"

struct spawn_transient_data {
  void *extradata;
  void (*callback)(uint8_t status, pid_t pid, void *extradata);
};

void _fs_root_dir_opened(VFatOpenFile* fp, uint8_t status, VFatOpenDir* dir, void* extradata)
{
  char filename_buffer[32];
  char attrs[16];

  memset(filename_buffer, 0, 32);
  memset(attrs, 0, 16);

  if(status!=0) {
    kprintf("ERROR Could not open root directory: error %d\r\n", status);
    vfat_close(fp);
    return;
  }
  kprintf("INFO Root directory contents:\r\n");
  DirectoryEntry *entry = vfat_read_dir_next(dir);
  while(entry!=NULL) {
    if(entry->attributes != VFAT_ATTR_LFNCHUNK) {
      vfat_get_printable_filename(entry, filename_buffer, 32);
      vfat_decode_attributes(entry->attributes, attrs);
      kprintf("DEBUG Found file %s %s size %l\r\n", entry, attrs, entry->file_size);
    }
    entry = vfat_read_dir_next(dir);
  }
  kprintf("INFO Root directory contents list completed\r\n");
  vfat_dir_close(dir);
  vfat_close(fp);
}

void _fs_shell_app_loaded(uint8_t status, struct elf_parsed_data* parsed_app, void *extradata)
{
  //Note: parsed_app is freed by the caller, don't free it here!
  if(status!=E_OK) {
    kprintf("ERROR Could not load SHELL.APP: error %d\r\n", (uint16_t)status);
    return;
  }
  kprintf("SHELL.APP loaded\r\n");

  pid_t pid = internal_create_process(parsed_app);
  kprintf("SHELL.APP running with PID %d\r\n", pid);

}

void _fs_shell_app_found(uint8_t status, DirectoryEntry *dir_entry, char *extradata)
{
  if(status!=E_OK) {
    kprintf("ERROR Could not find SHELL.APP in the root directory: error code %d\r\n", status);
    return;
  }
  if(dir_entry==NULL) {
    kprintf("ERROR SHELL.APP did not exist in the root directory.\r\n");
    return;
  }

  kprintf("Loading in root shell...\r\n");
  elf_load_and_parse(0, dir_entry, NULL, &_fs_shell_app_loaded);
}

void _fs_root_device_mounted(uint8_t status, const char *target, void *volume, void *extradata)
{
  kprintf("INFO Root FS mount completed with status %d\r\n", status);

  if(volume==NULL) {
    kprintf("ERROR Root volume pointer was NULL\r\n");
    return;
  }

  kprintf("INFO Starting search for SHELL.APP in %s\r\n", target);

  //vfat_find_8point3_in_root_dir(fs_ptr, "SHELL", "APP", NULL, &_fs_shell_app_found);
}

void mount_root_device()
{
  kputs("mount_root_device: pending deprecation");
  // kprintf("INFO Initialising FS of device 0\r\n");
  // FATFS* root_fs = fs_vfat_new(0, NULL, &_fs_root_device_mounted);
  // if(!root_fs) {
  //   k_panic("Unable to intialise root filesystem descriptor\r\n");
  // }
  // device_fs_map[0] = root_fs;
  // kprintf("INFO Starting mount of device 0\r\n");
  // root_fs->mount(root_fs, 0, NULL, &_fs_root_device_mounted);
}

/*
FS path convention:

$ide0p1:/path/to/file.ext
#root:/path/to/file.ext

$ denotes a concrete device - e.g., $ide0p1 is IDE device 0, partition 1
# denotes an "alias" - i.e., #root is the root device
Automatically set up aliases to the volume IDs when mounting volumes.

Devices and aliases are handled by the VolMgr module.
*/

/**
 * Resolves a filesystem path and calls the provided callback with the resulting FS and directory entry.
 */
uint8_t fs_resolve_path(const char *path, void *extradata, void (*callback)(uint8_t status, FATFS *fs_ptr, DirectoryEntry *dir_entry, void *extradata))
{
  const char *path_start = strchr(path, ':');

  if(!path_start) {
    kprintf("ERROR fs_resolve_path: path %s is not valid (no colon found)\r\n", path);
    return E_INVALID_FILE;
  }
  ++path_start;  //skip the colon

  FATFS *fs = volmgr_resolve_path_to_fs(path);
  if(!fs) {
    kprintf("ERROR fs_resolve_path: could not resolve path %s to fs\r\n", path);
    return E_INVALID_FILE;
  }

  const char *xtn = strchr(path_start, '.');
  vfat_find_8point3_in_root_dir(fs, (char *)path_start, xtn ? (char *)(xtn + 1) : "", extradata, callback);
  return E_OK;
}

void _fs_spawn_process_elf_loaded(uint8_t status, struct elf_parsed_data* parsed_app, void *extradata)
{
  struct spawn_transient_data *t = (struct spawn_transient_data *) extradata;
  //Note: parsed_app is freed by the caller, don't free it here!
  if(status!=E_OK) {
    kprintf("ERROR Could not load process ELF: error %d\r\n", (uint16_t)status);
    t->callback(status, -1, t->extradata);
    free(t);
    return;
  }
}

void _fs_spawn_process_file_found(uint8_t status, FATFS *fs_ptr, DirectoryEntry *dir_entry, void *extradata)
{
  char filename_buffer[32];
  memset(filename_buffer, 0, 32);

  struct spawn_transient_data *t = (struct spawn_transient_data *) extradata;
  if(status != E_OK) {
    kprintf("ERROR: Could not find file to spawn process from, code %d\r\n", status);
    t->callback(status, -1, t->extradata);
    free(t);
    return;
  }
  if(!dir_entry) {
    kprintf("ERROR: File to spawn process from does not exist\r\n");
    t->callback(E_INVALID_FILE, -1, t->extradata);
    free(t);
    return;
  }
  vfat_get_printable_filename(dir_entry, filename_buffer, 32);
  kprintf("INFO Spawning process from %s\r\n", filename_buffer);
  elf_load_and_parse(fs_ptr, dir_entry, t, &_fs_spawn_process_elf_loaded);
}

void spawn_process(const char *path, void *extradata, void (*callback)(uint8_t status, pid_t pid, void *extradata))
{
  struct spawn_transient_data *t = (struct spawn_transient_data *) malloc(sizeof(struct spawn_transient_data));
  if(!t) {
    callback(E_NOMEM, -1, extradata);
    return;
  }
  memset(t, 0, sizeof(struct spawn_transient_data));
  t->extradata = extradata;
  t->callback = callback;

  fs_resolve_path(path, (void *)t, &_fs_spawn_process_file_found);
}

void defer_launch_shell() {
  volmgr_register_callback("#root", "launch_shell", CB_MOUNT | CB_ONESHOT, NULL, &_fs_root_device_mounted);
}