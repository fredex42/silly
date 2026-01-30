lowlevel_o = custom_target('lowlevel.o',
  input: 'lowlevel.asm',
  output: 'lowlevel.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libcmos = static_library('cmos',
  sources: [
    'rtc.c',
  ],
  objects: [lowlevel_o],
  include_directories: inc,
)
