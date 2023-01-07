#include <types.h>

#ifndef __PROCESS_H
#define __PROCESS_H

#define PID_MAX 256
#define FILE_MAX 512

#define FP_TYPE_NONE    0
#define FP_TYPE_CONSOLE 1
#define FP_TYPE_VFAT    2

struct FilePointer {
  uint8_t type;
  void *content;  //the type of this pointer depends on the `type` field.
} __attribute__((packed));

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

  //open files
  struct FilePointer files[FILE_MAX];
} __attribute__((packed));


struct ProcessTableEntry* get_process(pid_t pid);
#endif
