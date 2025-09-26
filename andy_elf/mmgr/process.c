#include <types.h>
#include <sys/mmgr.h>
#include <sys/gdt.h>
#include <panic.h>
#include <spinlock.h>
#include <memops.h>
#include "heap.h"
#include "process.h"

//store a static pointer to the kernel process table
static struct ProcessTableEntry* process_table;
static uint16_t current_running_processid;
static uint16_t last_assigned_processid;
static spinlock_t process_table_lock = 0;

void initialise_process_table(uint32_t* kernel_paging_directory)
{
  kprintf("INFO Initialising process table\r\n");

  kprintf("DEBUG process_table_lock 0x%x\r\n", process_table_lock);
  process_table_lock = 0;
  
  size_t pages_required = (sizeof(struct ProcessTableEntry) * PID_MAX) / PAGE_SIZE;

  acquire_spinlock(&process_table_lock);
  process_table = (struct ProcessTableEntry*) vm_alloc_pages(NULL, pages_required+1, MP_PRESENT|MP_READWRITE|MP_GLOBAL);

  if(process_table==NULL) {
    k_panic("Unable to allocate memory for process table");
  }
  
  kprintf("INFO Process table is at 0x%x. Pages required 0x%x\r\n", process_table, pages_required+1);
  //memset(process_table, 0, sizeof(struct ProcessTableEntry)*PID_MAX);
  for(size_t p=0; p<pages_required+1; p++) {
    memset_dw((void *)((vaddr)process_table + (p*PAGE_SIZE)),0, PAGE_SIZE_DWORDS);
  }

  for(size_t i=0; i<PID_MAX; i++) {
    process_table[i].magic = PROCESS_TABLE_ENTRY_SIG;
    process_table[i].pid = (pid_t)i;
  }

  //process 0 is kernel
  current_running_processid = 0;
  //kernel paging directory is identity-mapped
  process_table->root_paging_directory_phys = kernel_paging_directory;
  process_table->root_paging_directory_kmem = kernel_paging_directory;
  process_table->status = PROCESS_READY;
  last_assigned_processid = 0;
  release_spinlock(&process_table_lock);
}

struct ProcessTableEntry* get_process(uint16_t pid)
{
  if(pid>PID_MAX) return NULL;

  //kprintf("DEBUG get_process: 0x%d\r\n", pid);

  acquire_spinlock(&process_table_lock);
  //kprintf("DEBUG process_table base 0x%x\r\n", process_table);
  struct ProcessTableEntry* e = &process_table[pid];
  // if(e >= 0x400000 && e<= 0x4001A0) {
  //   kprintf("DEBUG get_process: 0x%d\r\n", pid);
  //   kprintf("DEBUG sizeof ProcessTableEntry is 0x%x\r\n", sizeof(struct ProcessTableEntry));
  //   kprintf("DEBUG process_table base 0x%x\r\n", process_table);
  //   kprintf("DEBUG get_process entry for pid %d is 0x%x\r\n", pid, e);
  // }
  //kprintf("DEBUG get_process entry for pid %d is 0x%x\r\n", pid, e);
  
  if(e->magic!=PROCESS_TABLE_ENTRY_SIG) {
    kprintf("DEBUG get_process entry for pid %d is 0x%x\r\n", pid, e);
    k_panic("get_process Process table corruption detected\r\n");
  }
  release_spinlock(&process_table_lock);

  return e;
}

uint16_t get_current_processid()
{
  return current_running_processid;
}

struct ProcessTableEntry* get_current_process()
{
  return get_process(get_current_processid());
}

//INTERNAL USE ONLY! Called when entering kernel context.
uint16_t set_current_process_id(uint16_t pid)
{
  current_running_processid = pid;
}

/**
Scans the process table to find a free slot.
This should be treated as non-interruptable and therefore called with interrupts off.
*/
struct ProcessTableEntry *get_next_available_process()
{
  acquire_spinlock(&process_table_lock);

  for(uint16_t i=last_assigned_processid+1; i<PID_MAX; i++) {
    if(process_table[i].magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("get_next_available_process loop 1: Process table corruption detected\r\n");
    if(process_table[i].status==PROCESS_NONE) {
      last_assigned_processid = i;
      release_spinlock(&process_table_lock);
      return &process_table[i];
    }
  }
  //if we get here we hit PID_MAX, so start again from the beginning
  for(uint16_t i=1; i<=last_assigned_processid; i++) {
    if(process_table[i].magic!=PROCESS_TABLE_ENTRY_SIG) k_panic("get_next_available_process loop 2: Process table corruption detected\r\n");
    if(process_table[i].status==PROCESS_NONE) {
      last_assigned_processid = i;
      release_spinlock(&process_table_lock);
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
  void *phys_ptrs[5];

  cli();
  struct ProcessTableEntry *e = get_next_available_process();
  if(e==NULL) {
    return NULL;  //failed so bail out.
  }

  pid_t pid = e->pid;
  memset(e, 0, sizeof(struct ProcessTableEntry));
  e->magic  = PROCESS_TABLE_ENTRY_SIG;
  e->status = PROCESS_LOADING;
  e->pid = pid;

  //set up a paging directory
  kputs("DEBUG new_process Setting up paging directory\r\n");
  size_t c = allocate_free_physical_pages(4, phys_ptrs);
  if(c<4) {
    kprintf("ERROR Cannot allocate memory for new process\r\n");
    remove_process(e);
    return NULL;
  }
  e->root_paging_directory_phys = phys_ptrs[0];
  kprintf("DEBUG process paging directory at physical address 0x%x\r\n", e->root_paging_directory_phys);

  initialise_app_pagingdir(phys_ptrs, 4);
  e->root_paging_directory_kmem = NULL;

  //stack etc. are now set up in initialise_app_pagingdir
  //now set up stack at the end of the process's VRAM
  e->stack_page_count = 1;
  e->saved_regs.esp = 0xFFFFFFF8;
  e->stack_phys_ptr = phys_ptrs[3]; //FIXME - magic number usage - this corresponds to physical page 3 being set up as the stack page in initialise_app_pagingdir
  e->stack_kmem_ptr = (uint32_t *)k_map_next_unallocated_pages(MP_READWRITE, &phys_ptrs[3], 1);
  if(e->stack_kmem_ptr==NULL) {
    kputs("ERROR new_process could not map process stack into kmem for setup\r\n");
  }

  kputs("INFO process info now set up\r\n");
  //finally setup stin, stdout, stderr
  e->files[0].type = FP_TYPE_CONSOLE;
  e->files[0].read_buffer = ring_buffer_new(8);
  e->files[1].type = FP_TYPE_CONSOLE;
  e->files[2].type = FP_TYPE_CONSOLE;

  return e;
}

pid_t internal_create_process(struct elf_parsed_data *elf)
{
  struct ProcessTableEntry *new_entry = new_process();
  void **phys_ptrs;
  if(new_entry==NULL) {
    kputs("ERROR No process slots available!\r\n");
    return 0;
  }

  kputs("returned from new_process\r\n");
  uint32_t *mapped_pagedirs = map_app_pagingdir((vaddr)new_entry->root_paging_directory_phys, APP_PAGEDIRS_BASE);
  if(!mapped_pagedirs) {
    kputs("ERROR Unable to map app paging dir\r\n");
    new_entry->status = PROCESS_NONE;
    return 0;
  }

  for(size_t i=0; i<elf->program_headers_count; ++i) {
    if(elf->program_headers[i].p_type != PT_LOAD) continue;

    struct elf_program_header_i386 *ph = &elf->program_headers[i];
    size_t pages_required = ph->p_memsz / PAGE_SIZE;
    if((ph->p_memsz % PAGE_SIZE) != 0) pages_required++;  //if there is any remainder, we must allocate an extra page.
    if(pages_required==0) continue; //nothing to do!

    phys_ptrs = (void **)malloc(sizeof(void *) * pages_required);
    if(!phys_ptrs) {
      kputs("ERROR Unable to allocate memory for process segment\r\n");
      unmap_app_pagingdir(mapped_pagedirs);
      new_entry->status = PROCESS_NONE;
      return 0;
    }

    size_t c = allocate_free_physical_pages(pages_required, phys_ptrs);
    if(c<pages_required) {
      kputs("ERROR Unable to allocate memory for process segment\r\n");
      free(phys_ptrs);
      unmap_app_pagingdir(mapped_pagedirs);
      new_entry->status = PROCESS_NONE;
      return 0;
    }
    void *base_kernel_ptr = vm_map_next_unallocated_pages(NULL, MP_PRESENT|MP_USER|MP_READWRITE, phys_ptrs, pages_required);
    if(!base_kernel_ptr) {
      kputs("ERROR Unable to map process segment into kernel memory\r\n");
      deallocate_physical_pages(c, &phys_ptrs);
      free(phys_ptrs);
      unmap_app_pagingdir(mapped_pagedirs);
      new_entry->status = PROCESS_NONE;
      return 0;
    }
    memset_dw(base_kernel_ptr, 0, PAGE_SIZE_DWORDS * pages_required); //ensure that the memory is zeroed before we use it.
    //The entire segment should be in a single load list entry. We just need to find it.
    struct LoadList *seg = load_list_find_by_offset(elf->loaded_segments, ph->p_offset);
    if(!seg) {
      kprintf("ERROR internal_create_process could not find load list entry for segment %d\r\n", i);
      k_unmap_page_ptr(NULL, base_kernel_ptr);
      deallocate_physical_pages(c, &phys_ptrs);
      free(phys_ptrs);
      unmap_app_pagingdir(mapped_pagedirs);
      new_entry->status = PROCESS_NONE;
      return 0;
    }
    if(seg->length < ph->p_filesz) {
      kprintf("ERROR internal_create_process load list entry for segment %d is too small (%d < %d)\r\n", i, seg->length, ph->p_filesz);
      k_unmap_page_ptr(NULL, base_kernel_ptr);
      deallocate_physical_pages(c, &phys_ptrs);
      free(phys_ptrs);
      unmap_app_pagingdir(mapped_pagedirs);
      new_entry->status = PROCESS_NONE;
      return 0;
    }

    kprintf("DEBUG internal_create_process copying 0x%x bytes into 0x%x\r\n", ph->p_filesz, base_kernel_ptr);
    //There might be an offset from the start of the segment to the start of the load list entry.
    size_t offset_into_seg = ph->p_offset - seg->file_offset;
    kprintf("DEBUG source pointer is 0x%x dest is 0x%x (phys 0x%x)\r\n", (void *)((vaddr)seg->vptr + offset_into_seg), base_kernel_ptr, phys_ptrs[0]);

    memcpy(base_kernel_ptr, (void *)((vaddr)seg->vptr + offset_into_seg), ph->p_filesz);

    kprintf("DEBUG first bytes are 0x%x 0x%x 0x%x 0x%x\r\n", ((uint8_t *)base_kernel_ptr)[0], ((uint8_t *)base_kernel_ptr)[1], ((uint8_t *)base_kernel_ptr)[2], ((uint8_t *)base_kernel_ptr)[3]);

    //now map the pages into the process's paging directory and remove them from kernel space
    for(size_t pagenum=0;pagenum < pages_required; ++pagenum) {
      void *page_addr = (void *) ( (ph->p_vaddr & ~0x3FF) + pagenum*PAGE_SIZE);
      size_t flags = MP_PRESENT | MP_USER;
      if(ph->p_flags & SHF_WRITE) flags |= MP_READWRITE;
      k_map_page_bytes(mapped_pagedirs, phys_ptrs[pagenum], page_addr, flags);
      k_unmap_page_ptr(NULL, (vaddr)(base_kernel_ptr + pagenum*PAGE_SIZE));
    }
  }

  // kprintf("DEBUG app paging dirs remapped to 0x%x\r\n", mapped_pagedirs);

  // ElfLoadedSegment *seg = elf->loaded_segments_list;
  // size_t i=0;
  // while(seg!=NULL) {
  //   kprintf("INFO Segment %d is %d pages from src 0x%x\r\n", i, seg->page_count, seg->content);
  //   if(! seg->header ) {
  //     kprintf("ERROR internal_create_process section %d has no header\r\n", i);
  //     continue;
  //   }

  //   if(! (seg->header->p_flags) & SHF_ALLOC ) continue;

  //   kprintf("DEBUG internal_create_process mapping ELF section %d at 0x%x\r\n", i, seg->header->p_vaddr);
  //   void *base_kernel_ptr = NULL; //base_kernel_ptr points to the first of a set of allocated pages, mapped into kernel-space. this is so that they can be bulk-unmapped below.

  //   //first ensure that the RAM space is available
  //   for(size_t pagenum=0;pagenum < seg->page_count; ++pagenum) {
  //     void *page_addr = (void *) ( (seg->header->p_vaddr & ~0x3FF) + pagenum*PAGE_SIZE);
  //     if(!vm_is_address_present(mapped_pagedirs, page_addr)) {
  //       kprintf("DEBUG Address 0x%x is not mapped yet.\r\n", page_addr);
  //       size_t flags = MP_PRESENT| MP_READWRITE;  //we need readwrite access in order to set it up!
  //       //size_t flags = MP_USER|MP_PRESENT;
  //       //if(seg->header->p_flags & SHF_WRITE) flags |= MP_READWRITE;
  //       kprintf("DEBUG internal_create_process flags are 0x%x\r\n", flags);
  //       void *kernel_ptr = vm_alloc_specific_page(mapped_pagedirs, page_addr, flags);
  //       kprintf("DEBUG page mapped into kmem at 0x%x\r\n", kernel_ptr);
  //       if(!kernel_ptr) {
  //         kprintf("ERROR Unable to allocate memory for 0x%x in 0x%x\r\n", page_addr, mapped_pagedirs);
  //         k_panic("Aborting\r\n");  //FIXME - yeah should bail and cleanup, here.
  //       }
  //       if(base_kernel_ptr==NULL) base_kernel_ptr = kernel_ptr;
  //       memset_dw(kernel_ptr, 0, PAGE_SIZE_DWORDS);  //ensure that the page is blank before we use it.
  //     }
  //   }

  //   kprintf("DEBUG copying 0x%x bytes into 0x%x\r\n", seg->header->p_filesz, seg->header->p_vaddr);
  //   //now that the space is definitely there, copy the loaded data into it.  The pointers are freed at the end of this routine.
  //   memcpy(seg->header->p_vaddr, seg->content, seg->header->p_filesz);
  //   for(size_t pagenum=0;pagenum < seg->page_count; ++pagenum) {
  //     k_unmap_page_ptr(NULL, (vaddr)(base_kernel_ptr + pagenum*PAGE_SIZE));
  //   }

  //   // Update the paging flags to allow ring3 access and block write access if necessary
  //   for(size_t pagenum=0;pagenum < seg->page_count; ++pagenum) {
  //     void *page_addr = (void *) ( (seg->header->p_vaddr & ~0x3FF) + pagenum*PAGE_SIZE);
  //     size_t flags = MP_PRESENT | MP_USER;
  //     if(seg->header->p_flags & SHF_WRITE) flags |= MP_READWRITE;
  //     vm_update_page_flags(mapped_pagedirs, page_addr, flags);
  //   }
  //   i++;
  //   seg = seg->next;
  // }

  //now set up a heap
  kputs("DEBUG new_process setting up process heap\r\n");
  //the app prolog itself should configure the app heap, we just allocate it here.
  new_entry->heap_start = vm_alloc_pages(mapped_pagedirs, MIN_ZONE_SIZE_PAGES, MP_USER|MP_READWRITE);
  new_entry->heap_allocated = MIN_ZONE_SIZE_PAGES;
  new_entry->heap_used = 0;

  kprintf("DEBUG new_process heap is at 0x%x [process-space]\r\n", new_entry->heap_start);
  unmap_app_pagingdir(mapped_pagedirs);
  
  process_initial_stack(new_entry, elf->file_header);
  //the stack should now be ready for `iret`, we don't need access to it any more.
  kprintf("DEBUG new_process unmapping process stack at 0x%x from kernel\r\n", new_entry->stack_kmem_ptr);
  k_unmap_page_ptr(NULL, new_entry->stack_kmem_ptr);
  new_entry->stack_kmem_ptr = NULL;

  new_entry->status = PROCESS_READY;
  sti();
  kprintf("DEBUG new_process process initialised at 0x%x\r\n", new_entry);
  return new_entry->pid;
}

void process_initial_stack(struct ProcessTableEntry *new_entry, ElfFileHeader* file_header)
{
  kprintf("DEBUG process_initial_stack vaddr 0x%x\r\n", new_entry->stack_kmem_ptr);
  kprintf("DEBUG process_initial_stack paddr 0x%x\r\n", new_entry->stack_phys_ptr);
  
  //Finally, we must configure a stack frame so that the kernel can jump into the entrypoint of the app.
  //The jump is done by selecting the app paging directory, then its stack pointer and executing "IRET".
  //The "iret" instruction expects stack selector, stack pointer, eflags, code selector and execute address in that order.
  //See https://wiki.osdev.org/Getting_to_Ring_3#Entering_Ring_3
  uint32_t *process_stack_temp = (uint32_t *)((void *)new_entry->stack_kmem_ptr + 0x0FFC);  //one dword below top of stack
  *process_stack_temp = GDT_USER_DS | 3;  //user DS
  process_stack_temp -= 1;    //moves back 1 uint32_t i.e. 4 bytes
  *process_stack_temp = new_entry->saved_regs.esp - 0x04;           //process stack pointer, once return data is popped off
  process_stack_temp -= 1;
  *process_stack_temp = (1<<9);              //EFLAGS. Set IF is 1 and everything else 0.  IOPL is the permission level _required_ for sensitive stuff so keep at 0.
  process_stack_temp -= 1;
  *process_stack_temp = GDT_USER_CS | 3;  //user CS
  process_stack_temp -= 1;
  *process_stack_temp = file_header->i386_subheader.entrypoint;
  new_entry->saved_regs.esp -= 0x0C;

  kprintf("DEBUG process_initial_stack entrypoint at 0x%x\r\n", file_header->i386_subheader.entrypoint);
  mb();
}

void remove_process(struct ProcessTableEntry* e)
{
  //FIXME: need to de-alloc pages etc.
  e->status = PROCESS_NONE;
}
