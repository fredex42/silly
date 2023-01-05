#include <types.h>
#include <sys/mmgr.h>
#include <sys/gdt.h>
#include <panic.h>
#include "heap.h"
#include "process.h"

//store a static pointer to the kernel process table
static struct ProcessTableEntry* process_table;
static uint16_t current_running_processid;
static uint16_t last_assigned_processid;

void initialise_process_table(uint32_t* kernel_paging_directory)
{
  kprintf("INFO Initialising process table\r\n");
  size_t pages_required = (sizeof(struct ProcessTableEntry) * PID_MAX) / PAGE_SIZE;

  //kprintf("DEBUG require %d pages of RAM for process table\r\n", pages_required);
  process_table = (struct ProcessTableEntry*) vm_alloc_pages(NULL, pages_required+1, MP_PRESENT|MP_READWRITE);

  kprintf("INFO Process table is at 0x%x\r\n", process_table);
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
  //kprintf("DEBUG get_next_available_process process table at 0x%x, last assigned PID is %d\r\n", process_table, last_assigned_processid);
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
  ((uint32_t *)e->root_paging_directory_kmem)[0] = (uint32_t)phys_ptrs[1] | MP_PRESENT | MP_READWRITE; //we need kernel-only access to page 0 so that we can use interrupts

  //identity-map the kernel space so interrupts etc. will work
  uint32_t *page_one = (uint32_t *)k_map_next_unallocated_pages(MP_PRESENT|MP_READWRITE, &phys_ptrs[1], 1);
  //memset(page_one, 0, PAGE_SIZE);
  //mb();
  //FIXME... need MP_USER on the IDT but MP_READWRITE (NOT MP_USER) on the GDT. So they need to be on different pages
  //this page contains the GDT and IDT tables
  page_one[0] = MP_PRESENT | MP_READWRITE;  //kernel needs r/w in order to modify the access flag in the GDT. User-mode blocked.
  page_one[1] = (1 << 12) | MP_PRESENT | MP_USER;       //user-mode needs readonly in order to access IDT
  for(size_t i=2;i<256;i++) {
    page_one[i] = (i << 12) | MP_PRESENT | MP_READWRITE;  //r/w only applies to kernel here of course because no MP_USER
  }

  for(size_t i=256;i<1024;i++) {
    page_one[i] = 0;
  }

  //k_unmap_page_ptr(NULL, page_one);
  mb();

  //now set up stack at the end of the process's VRAM
  kputs("DEBUG new_process setting up process stack\r\n");
  void *process_stack = k_map_page(e->root_paging_directory_kmem, phys_ptrs[2], 1023, 1023, MP_USER|MP_READWRITE);
  kprintf("DEBUG new_process Set up 4k stack at 0x%x in process space\r\n", process_stack);
  e->stack_page_count = 1;
  e->esp = 0xFFFFFFFF;
  e->stack_phys_ptr = phys_ptrs[2];
  e->stack_kmem_ptr = (uint32_t *)k_map_next_unallocated_pages(MP_READWRITE, &phys_ptrs[2], 1);
  if(e->stack_kmem_ptr==NULL) {
    kputs("ERROR new_process could not map process stack into kmem for setup\r\n");
  }
  return e;
}

pid_t internal_create_process(struct elf_parsed_data *elf)
{
  struct ProcessTableEntry *new_entry = new_process();
  if(new_entry==NULL) {
    kputs("ERROR No process slots available!\r\n");
    return 0;
  }

  for(size_t i=0; i<elf->_loaded_segment_count; i++) {
    ElfLoadedSegment *seg = elf->loaded_segments[i];

    size_t flags = MP_USER|MP_PRESENT;
    if(seg->header->p_flags & SHF_WRITE) flags |= MP_READWRITE;
    kprintf("DEBUG internal_create_process mapping ELF section %d at 0x%x\r\n", i, seg->header->p_vaddr);
    kprintf("DEBUG internal_create_process flags are 0x%x\r\n", flags);
    for(size_t p=0; p<seg->page_count; ++p) {
      k_map_page_bytes(new_entry->root_paging_directory_kmem, seg->content_phys_pages[p], seg->header->p_vaddr + p*PAGE_SIZE, flags);
    }
    if(seg->content_virt_page!=NULL) {
      k_unmap_page_ptr(NULL, seg->content_virt_page);
      seg->content_virt_page = NULL;
      seg->content_virt_ptr = NULL;
    }
  }

  //now set up a heap
  kputs("DEBUG new_process setting up process heap\r\n");
  //the app prolog itself should configure the app heap, we just allocate it here.
  new_entry->heap_start = vm_alloc_pages(new_entry->root_paging_directory_kmem, MIN_ZONE_SIZE_PAGES, MP_USER|MP_READWRITE);
  new_entry->heap_allocated = MIN_ZONE_SIZE_PAGES;
  new_entry->heap_used = 0;

  kprintf("DEBUG new_process heap is at 0x%x [process-space]\r\n", new_entry->heap_start);

  //Finally, we must configure a stack frame so that the kernel can jump into the entrypoint of the app.
  //The jump is done by selecting the app paging directory, then its stack pointer and executing "IRET".
  //The "iret" instruction expects stack selector, stack pointer, eflags, code selector and execute address in that order.
  //See https://wiki.osdev.org/Getting_to_Ring_3#Entering_Ring_3
  uint32_t *process_stack_temp = (uint32_t *)((void *)new_entry->stack_kmem_ptr + 0x0FFB);  //one dword below top of stack
  *process_stack_temp = GDT_USER_DS | 3;  //user DS
  process_stack_temp -= 1;    //moves back 1 uint32_t i.e. 4 bytes
  *process_stack_temp = new_entry->esp - 0x04;           //process stack pointer, once return data is popped off
  process_stack_temp -= 1;
  *process_stack_temp = (1<<12) | (1<<13) | (1<<9);              //EFLAGS. Set IOPL to 3, IF is 1 and everything else 0.
  process_stack_temp -= 1;
  *process_stack_temp = GDT_USER_CS | 3;  //user CS
  process_stack_temp -= 1;
  *process_stack_temp = elf->file_header->i386_subheader.entrypoint;
  new_entry->esp -= 20;
  //the stack should now be ready for `iret`, we don't need access to it any more.
  k_unmap_page_ptr(NULL, new_entry->stack_kmem_ptr);
  new_entry->stack_kmem_ptr = NULL;

  new_entry->status = PROCESS_READY;
  sti();
  kprintf("DEBUG new_process process initialised at 0x%x\r\n", new_entry);
}

void remove_process(struct ProcessTableEntry* e)
{
  //FIXME: need to de-alloc pages etc.
  e->status = PROCESS_NONE;
}
