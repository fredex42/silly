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

//total size: 0x4C bytes
struct SavedRegisterStates32 {
  uint32_t eax;       //offset 0x00
  uint32_t ebx;       //offset 0x04
  uint32_t ecx;       //offset 0x08
  uint32_t edx;       //offset 0x0C
  uint32_t ebp;       //offset 0x10
  uint32_t esi;       //offset 0x14
  uint32_t edi;       //offset 0x18
  uint32_t eflags;    //offset 0x1C
  uint16_t cs;        //offset 0x20
  uint16_t ds;        //offset 0x22
  uint16_t es;        //offset 0x24
  uint16_t fs;        //offset 0x26
  uint16_t gs;        //offset 0x28
  uint32_t unused;    //offset 0x2A
  uint16_t unused2;   //offset 0x2E
  uint32_t dr0;       //offset 0x30
  uint32_t dr1;       //offset 0x34
  uint32_t dr2;       //offset 0x38
  uint32_t dr3;       //offset 0x3C
  uint32_t dr6;       //offset 0x40
  uint32_t dr7;       //offset 0x44
  uint32_t esp;       //offset 0x48
} __attribute__((packed));

struct ProcessTableEntry {
  uint32_t magic; //must be 0x54504552 = 'PTBE'               //offset 0x0
  //saved register states are on the process's stack
  size_t stack_page_count;                                    //offset 0x04

  //process state
  uint8_t status;                                             //offset 0x08
  uint8_t unused;                                             //offset 0x09
  uint8_t unused2;                                             //offset 0x0A
  uint8_t unused3;                                             //offset 0x0B

  //memory location and stats
  void* root_paging_directory_phys; //physical RAM address of the root paging directory. Offset 0x0C
  void* root_paging_directory_kmem; //virtual RAM address of the root paging directory, in kernel space. NULL if not currently mapped. Offset 0x10.
  void* heap_start;                 //location of the heap, in process space. Offset 0x14
  size_t heap_allocated;            //offset 0x18
  size_t heap_used;                 //offset 0x1C
  void *stack_phys_ptr;             //offset 0x30
  uint32_t *stack_kmem_ptr;         //offset 0x24

  struct SavedRegisterStates32 saved_regs;  //offset 0x28
  //open files
  struct FilePointer files[FILE_MAX];
  pid_t pid;
} __attribute__((packed));



struct ProcessTableEntry* get_process(pid_t pid);
#endif
