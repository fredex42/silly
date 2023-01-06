#include <types.h>

#ifndef __PROCESS_H
#define __PROCESS_H

#include <exeformats/elf.h>

#define PID_MAX 256

#define PROCESS_NONE    0
#define PROCESS_LOADING 1
#define PROCESS_READY   2
#define PROCESS_BUSY    3
#define PROCESS_IDLE    4
#define PROCESS_IOWAIT  5

#define PROCESS_TABLE_ENTRY_SIG 0x54504552

struct ProcessTableEntry {
  uint32_t magic; //must be 0x54504552 = 'PTBE'               //offset 0x0
  //saved register states are on the process's stack
  size_t esp;                                                 //offset 0x04
  uint16_t ecs;                                               //offset 0x08
  size_t stack_page_count;                                    //offset 0x0A 

  //process state
  uint8_t status;                                             //offset 0x0E
  uint8_t unused;                                             //offset 0x0F

  //memory location and stats
  void* root_paging_directory_phys; //physical RAM address of the root paging directory. Offset 0x10
  void* root_paging_directory_kmem; //virtual RAM address of the root paging directory, in kernel space. NULL if not currently mapped. Offset 0x14.
  void* heap_start;                 //location of the heap, in process space. Offset 0x18
  size_t heap_allocated;            //offset 0x1D
  size_t heap_used;                 //offset 0x22
  void *stack_phys_ptr;             //offset 0x26
  uint32_t *stack_kmem_ptr;         //offset 0x2A
} __attribute__((packed));

void initialise_process_table(uint32_t* kernel_paging_directory);
struct ProcessTableEntry* get_process(uint16_t pid);

pid_t internal_create_process(struct elf_parsed_data *elf);

#endif
