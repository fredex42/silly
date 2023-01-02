#include <types.h>
#include <sys/mmgr.h>
#include <panic.h>
#include "heap.h"
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

  kprintf("Process table is at 0x%x\r\n", process_table);
  memset(process_table, 0, sizeof(struct ProcessTableEntry)*PID_MAX);
  for(size_t i=0; i<PID_MAX; i++) {
    process_table[i].magic = PROCESS_TABLE_ENTRY_SIG;
  }

  //process 0 is kernel
  current_running_processid = 0;
  //kernel paging directory is identity-mapped
  process_table->root_paging_directory_phys = kernel_paging_directory;
  process_table->root_paging_directory_kmem = kernel_paging_directory;
  process_table->status = PROCESS_READY;
  last_assigned_processid = 0;
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
  kprintf("DEBUG get_next_available_process process table at 0x%x, last assigned PID is %d\r\n", process_table, last_assigned_processid);
  for(uint16_t i=last_assigned_processid+1; i<PID_MAX; i++) {
    if(process_table[i].magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("Process table corruption detected\r\n");
    if(process_table[i].status==PROCESS_NONE) {
      last_assigned_processid = i;
      return &process_table[i];
    }
  }
  //if we get here we hit PID_MAX, so start again from the beginning
  for(uint16_t i=1; i<=last_assigned_processid; i++) {
    if(process_table[i].magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("Process table corruption detected\r\n");
    if(process_table[i].status==PROCESS_NONE) {
      last_assigned_processid = i;
      return &process_table[i];
    }
  }
  //we ran out of PIDs!
  kprintf("ERROR Cannot start process, PID space is exhausted!\r\n");
  return NULL;
}

struct ProcessTableEntry* new_process()
{
  kprintf("INFO Initialising new process entry\r\n");
  void *phys_ptrs[3];

  cli();
  struct ProcessTableEntry *e = get_next_available_process();
  if(e==NULL) {
    sti();
    return NULL;  //failed so bail out.
  }

  memset(e, 0, sizeof(struct ProcessTableEntry));
  e->magic  = PROCESS_TABLE_ENTRY_SIG;
  e->status = PROCESS_LOADING;

  //set up a paging directory
  kputs("DEBUG new_process Setting up paging directory\r\n");
  size_t c = allocate_free_physical_pages(3, phys_ptrs);
  if(c<3) {
    kprintf("ERROR Cannot allocate memory for new process\r\n");
    remove_process(e);
    sti();
    return NULL;
  }
  e->root_paging_directory_phys = phys_ptrs[0];
  kprintf("DEBUG process paging directory at physical address 0x%x\r\n", e->root_paging_directory_phys);

  e->root_paging_directory_kmem = k_map_next_unallocated_pages(MP_PRESENT|MP_READWRITE, &e->root_paging_directory_phys, 1);
  ((uint32_t *)e->root_paging_directory_kmem)[0] = (uint32_t)phys_ptrs[1] | MP_PRESENT; //we need kernel-only access to page 0 so that we can use interrupts

  //identity-map the kernel space so interrupts etc. will work
  uint32_t *page_one = (uint32_t *)k_map_next_unallocated_pages(MP_PRESENT|MP_READWRITE, &phys_ptrs[1], 1);
  //memset(page_one, 0, PAGE_SIZE);
  //mb();
  for(size_t i=0;i<256;i++) {
    page_one[i] = (i << 12) | MP_PRESENT | MP_READWRITE;  //r/w only applies to kernel here of course because no MP_USER
    kprintf("[0x%x] = 0x%x\r\n", &page_one[i], page_one[i]);
  }
  for(size_t i=256;i<1024;i++) {
    page_one[i] = 0;
  }

  //k_unmap_page_ptr(NULL, page_one);
  mb();

  //now set up stack at the end of the process's VRAM
  kputs("DEBUG new_process setting up process stack\r\n");
  void *process_stack = k_map_page(e->root_paging_directory_kmem, phys_ptrs[2], 1023, 1023, MP_USER|MP_READWRITE);
  kprintf("DEBUG Set up 4k stack at 0x%x in process space\r\n", process_stack);
  e->stack_page_count = 1;
  e->esp = 0xFFFFFFFF;

  //now set up a heap
  kputs("DEBUG new_process setting up process heap\r\n");
  //the app prolog itself should configure the app heap, we just allocate it here.
  e->heap_start = vm_alloc_pages(e->root_paging_directory_kmem, MIN_ZONE_SIZE_PAGES, MP_USER|MP_READWRITE);
  kprintf("DEBUG new_process heap is at 0x%x [process-space]\r\n", e->heap_start);
  sti();
  kprintf("DEBUG new_process process initialised at 0x%x\r\n", e);
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
