#include <types.h>

#ifndef EXEC_ELF_FORMAT_H
#define EXEC_ELF_FORMAT_H

typedef struct ElfLoadedSegment {
  struct elf_section_header_i386 *header;
  size_t page_count;
  void **content_phys_pages;  //physical pages which contain the content. May be shared with other sections.
  void *content_virt_page;  //address of the page base in kernel virtual RAM. May be shared with other sections.
  void *content_virt_ptr;   //actual pointer to the data within the page - i.e. content_virt_page + offset.
} ElfLoadedSegment;

typedef struct elf_parsed_data {
  struct elf_file_header *file_header;
  struct elf_program_header_i386 *program_headers;
  size_t program_headers_count;

  void *section_headers_buffer;
  size_t section_headers_count;

  ElfLoadedSegment **loaded_segments;
  size_t _loaded_segment_count;
  size_t _scanned_segment_count;

  void *extradata;
  void (*callback)(uint8_t status, struct elf_parsed_data* parsed_data, void* extradata);
} ElfParsedData;

#endif
