#include <types.h>
#include <process.h>

#ifndef __MMGR_PROCESS_H
#define __MMGR_PROCESS_H

#include <exeformats/elf.h>

void initialise_process_table(uint32_t* kernel_paging_directory);
pid_t internal_create_process(struct elf_parsed_data *elf);

#endif
