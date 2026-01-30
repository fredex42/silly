native_api_o = custom_target('native_api.o',
  input: 'native_api.asm',
  output: 'native_api.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libnative_api = static_library('native_api',
  sources: [
    'process_ops.c',
    'stream_ops.c',
    'console.c',
  ],
  objects: [native_api_o],
  include_directories: inc,
)
