#include <types.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <fs/fat_dirops.h>
#include <memops.h>
#include <malloc.h>
#include <stdio.h>
#include "../drivers/ata_pio/ata_pio.h"
#include <fs.h>
#include <errors.h>
#include "../mmgr/process.h"
#include "../process/elfloader.h"
#include <scheduler/scheduler.h>

//We need to set up a shell process, once the root FS is mounted
void _poll_for_rootfs(SchedulerTask *t)
{
    size_t counter = (size_t)t->data;
    //get the pointer for the root filesystem
    FATFS *fsp = fs_for_device(0);
    if(fsp==NULL) { //root FS not yet mounted
        ++counter;
        t->data = (void *)counter;
        schedule_task(t);   //FIXME - we should probably delay better on this
        return;
    }

    //OK, we are now mounted
    kprintf("INFO poll_for_rootfs found root FS active after %d iterations\r\n", counter);

    vfat_find_8point3_in_root_dir(fsp, "SHELL", "APP", NULL, &_fs_shell_app_found);
    free(t);
}

void schedule_shell_startup()
{
    //should be TASK_AFTERTIME but that's not implemented yet...
    SchedulerTask *t = new_scheduler_task(TASK_ASAP, &_poll_for_rootfs, (void *)0);
    schedule_task(t);
}

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
  if(status!=E_OK) {
    kprintf("ERROR Could not load SHELL.APP: error %d\r\n", (uint16_t)status);
    return;
  }
  kprintf("SHELL.APP loaded\r\n");

  pid_t pid = internal_create_process(parsed_app);
  kprintf("SHELL.APP running with PID %d\r\n", pid);

  delete_elf_parsed_data(parsed_app);
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