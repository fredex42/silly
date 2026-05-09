picroutines_o = custom_target('picroutines.o',
  input: 'picroutines.asm',
  output: 'picroutines.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

interrupts_o = custom_target('interrupts.o',
  input: 'interrupts.asm',
  output: 'interrupts.o',
  command: [nasm, '-f', 'elf32', '-I', meson.project_source_root() + '/', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

lib8259pic = static_library('8259pic',
  sources: [
    '8259pic.c',
    'ioapic.c',
  ],
  objects: [picroutines_o, interrupts_o],
  include_directories: inc,
)
