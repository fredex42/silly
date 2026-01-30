basic_routines_o = custom_target('basic_routines.o',
  input: 'basic_routines.asm',
  output: 'basic_routines.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libata_pio = static_library('ata_pio',
  sources: [
    'detect.c',
    'readwrite.c',
    'interrupt.c',
  ],
  objects: [basic_routines_o],
  include_directories: inc,
)
