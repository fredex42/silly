#include <types.h>
#include <process.h>

#ifndef __MMGR_PROCESS_H
#define __MMGR_PROCESS_H

#include <exeformats/elf.h>

void initialise_process_table(uint32_t* kernel_paging_directory);
pid_t internal_create_process(struct elf_parsed_data *elf);
void remove_process(struct ProcessTableEntry* e);

//INTERNAL USE ONLY! Called by the scheduler when switching processes.
uint16_t set_current_process_id(uint16_t pid);
//Get the process table entry for a given pid
struct ProcessTableEntry* get_process(uint16_t pid);

#endif
