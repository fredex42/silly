#include <types.h>

#ifndef __ELFLOADER_H
#define __ELFLOADER_H

#define ELF_ABI_SYSV  0
#define ELF_ABI_HPUX  1
#define ELF_ABI_NETBSD  2
#define ELF_ABI_LINUX   3
#define ELF_ABI_HURD    4
#define ELF_ABI_SOLARIS 6
#define ELF_ABI_AIX     7
#define ELF_ABI_IRIX    8
#define ELF_ABI_FREEBSD 9
#define ELF_ABI_TRU64   0x0A
#define ELF_ABI_NOVELL_MODESTO  0x0B
#define ELF_ABI_OPENBSD 0x0C
#define ELF_ABI_OPENVMS 0x0D
#define ELF_ABI_NONSTOP 0x0E
#define ELF_ABI_AROS    0x0F
#define ELF_ABI_FENIXOS 0x10
#define ELF_ABI_CLOUDABI    0x11
#define ELF_ABI_OPENVOS 0x12
#define ELF_ABI_SILLY   0xF0

#define ET_NONE     0
#define ET_REL      1  //relocatable file
#define ET_EXEC     2  //executable file
#define ET_DYN      3  //shared object
#define ET_CORE     4  //core dump

#define ISA_I386    0x03
#define ISA_x86_64  0x3E

struct elf_i386_subheader {
  uint32_t version;
  uint32_t entrypoint;  //memory address to start executing
  uint32_t program_header_offset; //offset in the file to the program header table
  uint32_t section_header_offset;
  uint32_t flags;
  uint16_t file_header_size;
  uint16_t program_header_table_entry_size;
  uint16_t program_header_table_entry_count;
  uint16_t section_header_table_entry_size;
  uint16_t section_header_table_entry_count;
  uint16_t section_header_table_name_index;
} __attribute__((packed));

typedef struct elf_file_header {
  char magic[4];

  uint8_t ident_class;
  uint8_t ident_endian;
  uint8_t ident_version;
  uint8_t ident_os_abi;
  uint8_t ident_abi_version;
  char ident_pad[7];
  uint16_t e_type;
  uint16_t target_isa;

  union {
    struct elf_i386_subheader i386_subheader;
  };
} __attribute__((packed)) ElfFileHeader;

//values for p_type
#define PT_NULL       0   //unused
#define PT_LOAD       1   //loadable segment
#define PT_DYNAMIC    2   //dynamic linking information
#define PT_INTERP     3   //interpreter information
#define PT_NOTE       4   //auxiliary information
#define PT_SHLIB      5   //reserved
#define PT_PHDR       6   //segment containing the actual program header table
#define PT_TLS        7   //thread-local storage template

typedef struct elf_program_header_i386 {
  uint32_t p_type;
  uint32_t p_offset;     //location in the file
  uint32_t p_vaddr;      //where the segment should live in virtual memory
  uint32_t p_paddr;      //only for systems where physical address is relevant
  uint32_t p_filesz;     //size in bytes of the segment in the file image
  uint32_t p_memsz;      //size in bytes of the segment in memory
  uint32_t p_flags;       //depends on the segment type
  uint32_t p_align;      //0 and 1 specify no alignment. Otherwise should be a positive, integral power of 2, with p_vaddr equating p_offset modulus p_align.
} __attribute__((packed)) ElfProgramHeader32;

//values for sh_type
#define SHT_NULL      0   //unused section
#define SHT_PROGBITS  1   //program data
#define SHT_SYMTAB    2   //symbol table
#define SHT_STRTAB    3   //string table
#define SHT_RELA      4   //relocation entries with addends
#define SHT_HASH      5   //symbol hash table
#define SHT_DYNAMIC   6   //dynamic linking information
#define SHT_NOTE      7   //notes
#define SHT_NOBITS    8   //program space with no data (bss)
#define SHT_REL       9   //relocation entries without addends
#define SHT_SHLIB     0x0a //reserved
#define SHT_DYNSYM    0x0b //dynamic linker symbol table
#define SHT_INIT_ARRAY  0x0E  //array of constructors
#define SHT_FINI_ARRAY  0x0F  //array of destructors
#define SHT_PREINIT_ARRAY 0x10  //array of preconstructors
#define SHT_GROUP     0x11  //section group
#define SHT_SYMRAB_SHNDX  0x12  //extended section indices
#define SHT_NUM       0x13  //number of predefined types

//values for sh_flags
#define SHF_WRITE     0x01   //writable
#define SHF_ALLOC     0x02   //occupies memory during execution
#define SHF_EXECINSTR 0x04   //executable instructions
#define SHF_MERGE     0x10   //might be merged
#define SHF_STRINGS   0x20    //contains null-terminated strings
#define SHF_INFO_LINK 0x40   // 	'sh_info' contains SHT index
#define SHF_LINK_ORDER 0x80   //Preserve order after combining
#define SHF_OS_NONCONFORMING  0x100   //non-standard OS handling required
#define SHF_GROUP     0x200   //section is a member of a group
#define SHF_TLS       0x400   //section holds thread-local data

typedef struct elf_section_header_i386 {
  uint32_t sh_name;     // 	An offset to a string in the .shstrtab section that represents the name of this section.
  uint32_t sh_type;     //type of header
  uint32_t sh_flags;    //flags, as above
  uint32_t sh_addr;     //virtual address in memory, if the section is to be loaded
  uint32_t sh_offset;   //offset of the section in the file
  uint32_t sh_size;     //size of the section within the file, in bytes.
  uint32_t sh_link;     //section index of an associated section
  uint32_t sh_info;     //extra information about the section. Means different things depending on the type
  uint32_t sh_addralign;  //alignment requirement of the section. Must be a power of 2.
  uint32_t sh_entsize;    //if the section contains fixed-size entries, the size of each entry.
} __attribute__((packed)) ElfSectionHeader32;

#include <exeformats/elf.h>

#endif
