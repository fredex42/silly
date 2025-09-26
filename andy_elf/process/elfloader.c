#include <fs/fat_fileops.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <fs.h>
#include <stdio.h>
#include <errors.h>
#include <memops.h>
#include <sys/mmgr.h>
#include <malloc.h>
#include <exeformats/elf.h>
#include "elfloader.h"

struct LoadList *load_list_push(struct LoadList *list, struct LoadList *new) {
  if(!new) return list;
  if (!list) return new;

  struct LoadList *cur = list;
  while (cur->next) cur = cur->next;
  cur->next = new;
  return list;
}

size_t load_list_count(struct LoadList *list) {
  size_t count = 0;
  struct LoadList *cur = list;
  while (cur) {
    count++;
    cur = cur->next;
  }
  return count;
}

void delete_load_list(struct LoadList *list, uint8_t free_vptr) {
  struct LoadList *cur = list;
  while (cur) {
    struct LoadList *next = cur->next;
    if (cur->vptr && free_vptr) free(cur->vptr);
    free(cur);
    cur = next;
  }
}

void delete_elf_parsed_data(struct elf_parsed_data *t) {
  if(!t) return;
  if(t->file_header) free(t->file_header);
  if(t->program_headers) free(t->program_headers);
  free(t);
}

void delete_elf_loader_state(struct ElfLoaderState *s) {
  if(!s) return;
  if(s->load_list) delete_load_list(s->load_list, 1);
  if(s->parsed_data) delete_elf_parsed_data(s->parsed_data);
  free(s);
}

struct ElfLoaderState *new_elfloader_state(VFatOpenFile *file, void *extradata, void (*callback)(uint8_t status, struct elf_parsed_data* parsed_data, void* extradata)) {
  struct ElfLoaderState *s = (struct ElfLoaderState *) malloc(sizeof(struct ElfLoaderState));
  if(!s) return NULL;
  memset(s, 0, sizeof(struct ElfLoaderState));
  s->file = file;
  s->extradata = extradata;
  s->callback = callback;
  return s;
}

void _elf_loaded_program_headers(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata) {
  struct elf_program_header_i386 *headers = (struct elf_program_header_i386 *) buf;
  struct ElfLoaderState *t = (struct ElfLoaderState *) extradata;
  if(status != E_OK) {
    kprintf("ERROR: Could not read ELF program headers, code %d\r\n", status);
    t->callback(status, t->parsed_data, t->extradata);
    delete_elf_loader_state(t);
    if(headers) free(headers);
    return;
  }
  
  t->parsed_data->program_headers = headers;
  t->parsed_data->program_headers_count = t->parsed_data->file_header->i386_subheader.program_header_table_entry_count;
  kprintf("INFO Loaded %d ELF program headers\r\n", t->parsed_data->program_headers_count);

  //Now we must build a load-list
  for(size_t i = 0; i < t->parsed_data->program_headers_count; i++) {
    struct LoadList *entry = (struct LoadList *) malloc(sizeof(struct LoadList));
    if(!entry) {
      kprintf("ERROR: Out of memory\r\n");
      t->callback(E_NOMEM, t->parsed_data, t->extradata);
      delete_elf_loader_state(t);
      return;
    }
    memset(entry, 0, sizeof(struct LoadList));
    entry->file_offset = t->parsed_data->program_headers[i].p_offset;
    entry->length = t->parsed_data->program_headers[i].p_filesz;
    t->load_list = load_list_push(t->load_list, entry);
  }

  //We have a basic load list; now we should coalesce any adjacent entries
  for(struct LoadList *cur = t->load_list; cur != NULL; cur = cur->next) {
    while(cur->next && (cur->file_offset + cur->length == cur->next->file_offset)) {
      struct LoadList *to_delete = cur->next;
      cur->length += to_delete->length;
      cur->next = to_delete->next;
      free(to_delete);
    }
  }

  kprintf("INFO After coalesce, we have %d load list entries\r\n", load_list_count(t->load_list));

}

void _elf_loaded_file_header(VFatOpenFile *fp, uint8_t status, size_t bytes_read, void *buf, void* extradata) {
  ElfFileHeader *header = (ElfFileHeader *) buf;
  struct ElfLoaderState *t = (struct ElfLoaderState *) extradata;
  if(status != E_OK) {
    kprintf("ERROR: Could not read ELF file header, code %d\r\n", status);
    t->callback(status, t->parsed_data, t->extradata);
    free(t);
    return;
  }

  //check the magic
  if(header->magic[0] != 0x7F || header->magic[1] != 'E' || header->magic[2] != 'L' || header->magic[3] != 'F') {
    kprintf("ERROR: Not a valid ELF file (bad magic)\r\n");
    t->callback(E_NOT_ELF, t, t->extradata);
    free(t);
    free(header);
    return;
  }

  //check the class
  if(header->ident_class != 1) {
    kprintf("ERROR: Only 32-bit ELF files are supported\r\n");
    t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  //check the endianness
  if(header->ident_endian != 1) {
    kprintf("ERROR: Only little-endian ELF files are supported\r\n");
    t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  //check the version
  if(header->ident_version != 1) {
    kprintf("ERROR: Unsupported ELF version %d\r\n", header->ident_version);
    t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  // //check the OS ABI
  // if(header->ident_os_abi != ELF_ABI_SILLY && header->ident_os_abi != ELF_ABI_LINUX) {
  //   kprintf("ERROR: Unsupported ELF OS ABI %d\r\n", header->ident_os_abi);
  //   t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
  //   free(t);
  //   free(header);
  //   return;
  // }
  //check the ABI version
  if(header->ident_abi_version != 0) {
    kprintf("ERROR: Unsupported ELF ABI version %d\r\n", header->ident_abi_version);
    t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  //check the type
  if(header->e_type != ET_EXEC && header->e_type != ET_DYN) {
    kprintf("ERROR: Only executable and shared-object ELF files are supported (type %d)\r\n", header->e_type);
    t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  //check the target ISA
  if(header->target_isa != ISA_I386) {
    kprintf("ERROR: Only i386 ELF files are supported (target ISA %d)\r\n", header->target_isa);
    t->callback(E_UNSUPPORTED_ELF, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  kprintf("INFO ELF file header looks good\r\n");
  t->parsed_data = (struct elf_parsed_data *) malloc(sizeof(struct elf_parsed_data));
  if(!t->parsed_data) {
    kprintf("ERROR: Out of memory\r\n");
    t->callback(E_NOMEM, t->parsed_data, t->extradata);
    free(t);
    free(header);
    return;
  }
  memset(t->parsed_data, 0, sizeof(struct elf_parsed_data));
  t->parsed_data->file_header = header;
  kprintf("INFO ELF entry point is 0x%x\r\n", header->i386_subheader.entrypoint);
  kprintf("INFO ELF has %d program headers, at offset 0x%x\r\n", header->i386_subheader.program_header_table_entry_count, header->i386_subheader.program_header_offset);
  if(header->i386_subheader.program_header_table_entry_count == 0) {
    kprintf("ERROR: ELF file has no program headers\r\n");
    t->callback(E_MALFORMED_ELF, t->parsed_data, t->extradata);
    free(t->parsed_data);
    free(t);
    return;
  }
  //load in the program headers
  size_t ph_size = header->i386_subheader.program_header_table_entry_count * sizeof(ElfProgramHeader32);
  ElfProgramHeader32 *ph_buf = (ElfProgramHeader32 *) malloc(ph_size);
  if(!ph_buf) {
    kprintf("ERROR: Out of memory\r\n");
    t->callback(E_NOMEM, t->parsed_data, t->extradata);
    free(t->parsed_data);
    free(t);
    return;
  }
  memset(ph_buf, 0, ph_size);
  kprintf("INFO Loading ELF program headers...\r\n");
  uint8_t err = vfat_seek(fp, t->parsed_data->file_header->i386_subheader.program_header_offset, SEEK_SET);
  if(err != 0) {
    kprintf("ERROR: Could not seek to program headers, code %d\r\n", err);
    t->callback(err, t->parsed_data, t->extradata);
    free(t->parsed_data);
    free(t);
    free(ph_buf);
    return;
  }

  vfat_read_async(fp, (void *)ph_buf, ph_size, (void*)t, &_elf_loaded_program_headers);
}


void elf_load_and_parse(uint8_t device_index, DirectoryEntry *file, void *extradata, void (*callback)(uint8_t status, ElfParsedData* something, void *extradata))
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

  struct ElfLoaderState *t = (struct ElfLoaderState *) malloc(sizeof(struct ElfLoaderState));
  if(!t) {
    callback(E_NOMEM, NULL, extradata);
    return;
  }
  memset(t, 0, sizeof(struct ElfLoaderState));
  t->extradata = extradata;
  t->callback = callback;
  t->file = fp;

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
