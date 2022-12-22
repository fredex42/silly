#include <fs/fat_fileops.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <fs.h>
#include <stdio.h>
#include <errors.h>
#include <memops.h>
#include <malloc.h>
#include "elfloader.h"

struct elf_load_transient_data {
  ElfFileHeader *file_header;
  ElfProgramHeader32 *program_header;
  void *section_headers_buffer;
  size_t section_headers_count;

  void *extradata;
  void (*callback)(uint8_t status, void*something, void*extradata);
};

/**
Returns an ElfSectionHeader32 struct from the buffer in `t`.
Returns NULL if the index is invalid.
*/
ElfSectionHeader32* elf_get_section_header_by_index(struct elf_load_transient_data *t, size_t idx)
{
  if(idx>t->section_headers_count) return NULL;
  ElfSectionHeader32 *section = (ElfSectionHeader32 *)(t->section_headers_buffer + idx*sizeof(ElfSectionHeader32));
  return section;
}

/**
Invokes the provided callback once for each section
*/
uint32_t elf_sections_foreach(struct elf_load_transient_data *t, void *extradata, void (*callback)(struct elf_load_transient_data *t, void *extradata, uint32_t idx, ElfSectionHeader32* section))
{
  size_t i;
  if(!t->section_headers_buffer) return 0;
  ElfSectionHeader32 *section = (ElfSectionHeader32 *)t->section_headers_buffer;

  for(i=0; i<t->section_headers_count; i++) {
    callback(t, extradata, i, section);
    section++;
  }
  return i;
}

void elf_load_program_header(VFatOpenFile *fp, struct elf_load_transient_data *t)
{
  kprintf("Loading in program header...\r\n");
  if(!t->file_header) {
    kprintf("ERROR elf_load_program_header called with NULL file header, can't continue");
    return;
  }
  void *program_header_buffer = malloc(t->file_header->i386_subheader.program_header_table_entry_size);
}

void _elf_show_section(struct elf_load_transient_data *t, void *extradata, uint32_t idx, ElfSectionHeader32 *section)
{
  kprintf("ELF section %d:\r\n", idx);
  kprintf("    Type is %d, flags are 0x%x, load address 0x%x, file offset 0x%x\r\n", section->sh_type, section->sh_flags, section->sh_addr, section->sh_offset);
}

void _elf_loaded_section_headers(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata) {
  struct elf_load_transient_data *t = (struct elf_load_transient_data *)extradata;
  uint8_t failed = 0;

  if(status!=E_OK) {
    kprintf("ERROR: Could not load section headers, error code was 0x%x\r\n", (uint32_t)status);
    t->callback(status, t, t->extradata);
    if(t->file_header) free(t->file_header);
    if(t->program_header) free(t->program_header);
    free(t);
    return;
  }

  kprintf("DEBUG: Section headers in buffer at 0x%x\r\n", buf);
  t->section_headers_buffer = buf;
  elf_sections_foreach(t, NULL, &_elf_show_section);
  //elf_load_program_header(fp, t);
}

void elf_load_section_headers(VFatOpenFile *fp, struct elf_load_transient_data *t)
{
  kprintf("Loading in section headers...\r\n");
  if(!t->file_header) {
    kprintf("ERROR elf_load_section_headers called with NULL file header, can't continue");
    return;
  }

  size_t header_block_length = t->file_header->i386_subheader.section_header_table_entry_size * t->file_header->i386_subheader.section_header_table_entry_count;
  void *section_header_buffer = malloc(header_block_length);
  if(!section_header_buffer) {
    kprintf("ERROR Unable to allocate memory for section headers\r\n");
    return;
  }
  t->section_headers_count = t->file_header->i386_subheader.section_header_table_entry_count;
  vfat_seek(fp, t->file_header->i386_subheader.section_header_offset, SEEK_SET);

  kprintf("DEBUG Loading in %l section headers of %l bytes each\r\n", t->file_header->i386_subheader.section_header_table_entry_count, t->file_header->i386_subheader.section_header_table_entry_size);
  vfat_read_async(fp, section_header_buffer, header_block_length, (void *)t, &_elf_loaded_section_headers);
}

void _elf_loaded_file_header(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata) {
  struct elf_load_transient_data *t = (struct elf_load_transient_data *)extradata;
  uint8_t failed = 0;
  kprintf("File header loaded (%l bytes), inspecting...\r\n", bytes_read);

  if(status!=E_OK) {
    if(buf) free(buf);
    vfat_close(fp);
    t->callback(status, NULL, extradata);
    free(t);
    return;
  }

  t->file_header = (ElfFileHeader *) buf;
  kprintf("ELF file header at 0x%x in memory\r\n", t->file_header);

  if(t->file_header->magic[0] != 0x7F || t->file_header->magic[1] != 0x45 ||
     t->file_header->magic[2] != 0x4c || t->file_header->magic[3] != 0x46) {
    kprintf("ELF magic number check failed, this is not an executable. Got 0x%x\r\n",
      (uint32_t)t->file_header->magic[1]
    );
    if(buf) free(buf);
    vfat_close(fp);
    t->callback(E_BAD_EXECUTABLE, NULL, extradata);
    free(t);
    return;
  }

  if(! (t->file_header->ident_os_abi == ELF_ABI_LINUX || t->file_header->ident_os_abi == ELF_ABI_SYSV)) {  //kinda temporary but that is what we expect for now
    kprintf("ELF executable had wrong ABI, expected linux (3) got %d\r\n", t->file_header->ident_os_abi);
    failed = 1;
  }
  if(t->file_header->ident_endian != 1) {
    kprintf("Expected little-endian ELF format but got big.\r\n");
    failed = 1;
  }
  if(t->file_header->ident_class != 1) {
    kprintf("Expected 32-bit executable (1), got class %d\r\n", t->file_header->ident_class);
    failed = 1;
  }
  if(t->file_header->e_type != ET_EXEC) {
    kprintf("This is not an executable file. Expected 2 got %d\r\n", t->file_header->e_type);
    failed = 1;
  }
  if(t->file_header->target_isa != 0x03) {
    kprintf("We only support x86 (i386) executables at present\r\n");
    failed = 1;
  }
  if(failed == 1) {
    if(buf) free(buf);
    vfat_close(fp);
    t->callback(E_BAD_EXECUTABLE, NULL, extradata);
    free(t);
    return;
  }

  kprintf("Executable looks OK, starting load...\r\n");
  elf_load_section_headers(fp, t);
}

void elf_load_and_parse(uint8_t device_index, DirectoryEntry *file, void *extradata, void (*callback)(uint8_t status, void*something, void *extradata))
{
  const FATFS* fs_ptr = fs_for_device(device_index);
  if(!fs_ptr) {
    callback(E_INVALID_DEVICE, NULL, extradata);
    return;
  }

  VFatOpenFile *fp = vfat_open(fs_ptr, file);
  if(!fp) {
    callback(E_INVALID_FILE, NULL, extradata);
    return;
  }

  struct elf_load_transient_data *t = (struct elf_load_transient_data *) malloc(sizeof(struct elf_load_transient_data));
  if(!t) {
    callback(E_NOMEM, NULL, extradata);
    return;
  }
  memset(t, 0, sizeof(struct elf_load_transient_data));
  t->extradata = extradata;
  t->callback = callback;

  /* Step one - load in the file header */
  ElfFileHeader *header_buf = (ElfFileHeader *)malloc(sizeof(ElfFileHeader));
  if(!header_buf) {
    callback(E_NOMEM, NULL, extradata);
    return;
  }
  memset(header_buf, 0, sizeof(ElfFileHeader));

  kprintf("INFO Loading ELF file header...\r\n");
  vfat_read_async(fp, (void *)header_buf, sizeof(ElfFileHeader), (void*)t, &_elf_loaded_file_header);
}
