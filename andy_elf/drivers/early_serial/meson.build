serial_lowlevel_o = custom_target('serial_lowlevel.o',
  input: 'serial_lowlevel.asm',
  output: 'serial_lowlevel.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libearly_serial = static_library('early_serial',
  objects: [serial_lowlevel_o],
)
