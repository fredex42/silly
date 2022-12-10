#include <types.h>
#include <sys/mmgr.h>
#include <panic.h>
#include "process.h"

//store a static pointer to the kernel process table
static struct ProcessTableEntry* process_table;

void initialise_process_table(uint32_t* kernel_paging_directory)
{
  kprintf("Initialising process table\r\n");
  size_t pages_required = (sizeof(struct ProcessTableEntry) * PID_MAX) / PAGE_SIZE;

  kprintf("DEBUG require %d pages of RAM for process table\r\n", pages_required);
  process_table = (struct ProcessTableEntry*) vm_alloc_pages(NULL, pages_required+1, MP_PRESENT|MP_READWRITE);

  struct ProcessTableEntry *e;
  e = process_table;
  for(size_t i=0; i<PID_MAX; i++) {
    memset(e, 0, sizeof(struct ProcessTableEntry));
    e->magic = PROCESS_TABLE_ENTRY_SIG;
  }
  kprintf("Process table is at 0x%x\r\n", process_table);

  //process 0 is kernel
  process_table->root_paging_directory = kernel_paging_directory;
  process_table->status = PROCESS_READY;
}

struct ProcessTableEntry* get_process(uint16_t pid)
{
  if(pid>PID_MAX) return NULL;

  size_t offset = pid * sizeof(struct ProcessTableEntry);
  struct ProcessTableEntry* e = (struct ProcessTableEntry *)((char *)process_table + offset);
  if(e->magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("Process table corruption detected\r\n");
  return e;
}
