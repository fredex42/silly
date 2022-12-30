#include <types.h>
#include <sys/mmgr.h>
#include <panic.h>
#include "process.h"

//store a static pointer to the kernel process table
static struct ProcessTableEntry* process_table;
static uint16_t current_running_processid;
static uint16_t last_assigned_processid;

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
  current_running_processid = 0;
  //kernel paging directory is identity-mapped
  process_table->root_paging_directory_phys = kernel_paging_directory;
  process_table->root_paging_directory_kmem = kernel_paging_directory;
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

/**
Scans the process table to find a free slot.
This should be treated as non-interruptable and therefore called with interrupts off.
*/
struct ProcessTableEntry *get_next_available_process()
{
  for(uint16_t i=last_assigned_processid+1; i<PID_MAX; i++) {
    size_t offset = i * sizeof(struct ProcessTableEntry);
    struct ProcessTableEntry* e = (struct ProcessTableEntry *)((char *)process_table + offset);
    if(e->magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("Process table corruption detected\r\n");
    if(e->status==PROCESS_NONE) {
      last_assigned_processid = i;
      return e;
    }
  }
  //if we get here we hit PID_MAX, so start again from the beginning
  for(uint16_t i=1; i<=last_assigned_processid; i++) {
    size_t offset = i * sizeof(struct ProcessTableEntry);
    struct ProcessTableEntry* e = (struct ProcessTableEntry *)((char *)process_table + offset);
    if(e->magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("Process table corruption detected\r\n");
    if(e->status==PROCESS_NONE) {
      last_assigned_processid = i;
      return e;
    }
  }
  //we ran out of PIDs!
  kprintf("ERROR Cannot start process, PID space is exhausted!\r\n");
  return NULL;
}

struct ProcessTableEntry* new_process()
{
  kprintf("INFO Initialising new process entry");

  cli();
  struct ProcessTableEntry *e = get_next_available_process();
  if(e==NULL) {
    sti();
    return NULL;  //failed so bail out.
  }

  memset(e, 0, sizeof(struct ProcessTableEntry));
  e->magic  = PROCESS_TABLE_ENTRY_SIG;
  e->status = PROCESS_LOADING;

  size_t c = allocate_free_physical_pages(1, &e->root_paging_directory_phys);
  if(c!=1) {
    kprintf("ERROR Cannot allocate memory for new process\r\n");
    remove_process(e);
    sti();
    return NULL;
  }
  e->root_paging_directory_kmem = k_map_next_unallocated_pages(MP_PRESENT, &e->root_paging_directory_phys, 1);
  sti();

  return e;
}

pid_t internal_create_process(struct elf_parsed_data *elf)
{
  struct ProcessTableEntry *new_entry = new_process();
  if(new_entry==NULL) {
    kputs("ERROR No process slots available!\r\n");
    return 0;
  }

  
}
void remove_process(struct ProcessTableEntry* e)
{
  //FIXME: need to de-alloc pages etc.
  e->status = PROCESS_NONE;
}
