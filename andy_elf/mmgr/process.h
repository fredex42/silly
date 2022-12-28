#include <types.h>

#ifndef __PROCESS_H
#define __PROCESS_H

#define PID_MAX 256

#define PROCESS_NONE    0
#define PROCESS_LOADING 1
#define PROCESS_READY   2
#define PROCESS_BUSY    3
#define PROCESS_IDLE    4
#define PROCESS_IOWAIT  5

#define PROCESS_TABLE_ENTRY_SIG 0x54504552

struct ProcessTableEntry {
  uint32_t magic; //must be 0x54504552 = 'PTBE'
  //saved register states are on the process's stack
  size_t esp;
  uint16_t ecs;
  size_t stack_page_count;

  //process state
  uint8_t status;

  //memory location and stats
  void* root_paging_directory_phys; //physical RAM address of the root paging directory
  void* root_paging_directory_kmem; //virtual RAM address of the root paging directory, in kernel space.
  void* heap_start;                 //location of the heap, in process space
  size_t heap_allocated;
  size_t heap_used;
} __attribute__((packed));

void initialise_process_table(uint32_t* kernel_paging_directory);
struct ProcessTableEntry* get_process(uint16_t pid);

#endif
