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

  void *extradata;
  void (*callback)(uint8_t status, void*something, void*extradata);
};

void elf_load_section_headers(VFatOpenFile *fp, struct elf_load_transient_data *t)
{
  kprintf("Loading in section headers...\r\n");
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
