#include <types.h>
#include <process.h>
#include <scheduler/scheduler.h>
#include <stdio.h>
#include <native_api/errors.h>
#include <fs/fat_fileops.h>
#include <drivers/kb_buffer.h>
#include "stream_ops.h"
#include "console.h"

uint32_t api_open(char *filename, char *xtn, uint16_t mode_flags)
{

}

void api_close(uint32_t fd)
{

}

void _api_process_read_completed(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void *extradata)
{
  kprintf("INFO _api_process_read_completed FIXME not implemented yet!\r\n");
}

size_t api_read(uint32_t fd, char *buf, size_t len)
{
  char ch;

  pid_t current_pid = get_active_pid();
  kprintf("DEBUG api_read on file 0x%x current PID is 0x%d\r\n", fd, current_pid);
  
  if(fd > FILE_MAX) return API_ERR_NOTSUPP;

  if(current_pid==0) {
    return API_ERR_NOTSUPP;
  }

  struct ProcessTableEntry *process = get_process(current_pid);
  if(!process || process->status==PROCESS_NONE) {
    return API_ERR_NOTSUPP;
  }
  if(process->magic!=PROCESS_TABLE_ENTRY_SIG) {
    kprintf("ERROR api_read process entry for %d is not valid, process table may be corrupted!\r\n", current_pid);
    return API_ERR_CONSISTENCY;
  }

  struct FilePointer *fp = &process->files[fd];
  switch(fp->type) {
    case FP_TYPE_NONE:
      return API_ERR_NOTSUPP;
    case FP_TYPE_CONSOLE:
      return kb_read_to_file(process, fp);
    case FP_TYPE_VFAT:
      //FIXME: callbacks will be run in kernel context, so we need to translate the buffer into kernel space first.
      vfat_read_async((VFatOpenFile *)fp->content, buf, len, (void *)process, &_api_process_read_completed);
      process->status = PROCESS_IOWAIT;
      //FIXME: we are going into IOWAIT so need to switch context
      return 0;
    default:
      kprintf("ERROR api_read fd %l for process %d is not valid, unknown type\r\n", fd, current_pid);
      return API_ERR_CONSISTENCY;
  }
}

size_t api_write(uint32_t fd, char *buf, size_t len)
{
  if(fd > FILE_MAX) return API_ERR_NOTSUPP;
  pid_t current_pid = get_active_pid();
  kprintf("DEBUG api_write current PID is 0x%d\r\n", current_pid);
  if(current_pid==0) {
    return API_ERR_NOTSUPP;
  }

  struct ProcessTableEntry *process = get_process(current_pid);
  if(!process || process->status==PROCESS_NONE) {
    return API_ERR_NOTSUPP;
  }
  if(process->magic!=PROCESS_TABLE_ENTRY_SIG) {
    kprintf("ERROR api_write process entry for %d is not valid, process table may be corrupted!\r\n", current_pid);
    return API_ERR_CONSISTENCY;
  }

  struct FilePointer *fp = &process->files[fd];
  switch(fp->type) {
    case FP_TYPE_NONE:
      return API_ERR_NOTSUPP;
    case FP_TYPE_CONSOLE:
      return console_write(buf, len);
    case FP_TYPE_VFAT:
      //FIXME: VFAT file write has not been implemented in the fs driver yet
      return API_ERR_NOTSUPP;
    default:
      kprintf("ERROR api_write fd %l for process %d is not valid, unknown type\r\n", fd, current_pid);
      return API_ERR_CONSISTENCY;
  }
}
