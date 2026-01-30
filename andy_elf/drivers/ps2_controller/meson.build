lowlevel_o = custom_target('ps2_lowlevel.o',
  input: 'lowlevel.asm',
  output: 'ps2_lowlevel.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libps2_controller = static_library('ps2_controller',
  sources: [
    'controller.c',
    'keymap.c',
  ],
  objects: [lowlevel_o],
  include_directories: inc,
)
