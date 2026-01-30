string_o = custom_target('string.o',
  input: 'string.asm',
  output: 'string.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libutils = static_library('utils',
  sources: [
    'ucs_conv.c',
    'spinlock.c',
    'ringbuffer.c',
    'debug.c',
  ],
  objects: [string_o],
  include_directories: inc,
)
