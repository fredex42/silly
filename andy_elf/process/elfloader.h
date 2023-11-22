#include <types.h>
#include <exeformats/elf.h>

#ifndef __ELFLOADER_H
#define __ELFLOADER_H

/**
 * Removes an ELF parsed data container and all references within it
*/
void delete_elf_parsed_data(struct elf_parsed_data *t);

uint32_t elf_sections_foreach(struct elf_parsed_data *t, void *extradata, void (*callback)(struct elf_parsed_data *t, void *extradata, uint32_t idx, ElfSectionHeader32* section));

/**
 * The main function. Loads the given ELF file into RAM and parses the contents into an elf_parsed_data structure.
 * This is asynchronous, so no value is returned directly; you must specify a callback to receive data once the load is completed.
*/
void elf_load_and_parse(uint8_t device_index, DirectoryEntry *file, void *extradata, void (*callback)(uint8_t status, ElfParsedData* something, void *extradata));

#endif
