project('andy_elf', ['c'], version: '0.1', default_options: ['c_std=gnu11', 'warning_level=0'])

# Toolchain and global flags
cc = meson.get_compiler('c')
nasm = find_program('nasm', required: true)
bash = find_program('bash', required: true)

common_c_args = [
  '-fno-pie',
  '-m32',
  '-march=i486',
  '-fno-asynchronous-unwind-tables',
  #'-fno-stack-protector',
  '-O0',
]
add_project_arguments(common_c_args, language: 'c')
add_project_arguments(['-std=gnu11'], language: 'c')
inc = include_directories('include')

# Build libraries in subdirectories
subdir('mmgr')
subdir('8259pic')
subdir('acpi')
subdir('drivers')
subdir('scheduler')
subdir('native_api')
subdir('volmgr')
subdir('fs')
subdir('process')
subdir('utils')

# NASM-assembled objects in the root
kickoff_o = custom_target('kickoff.o',
  input: 'kickoff.s',
  output: 'kickoff.o',
  command: [nasm, '-f', 'elf32', '-I', meson.current_source_dir() + '/', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

basic_console_o = custom_target('basic_console.o',
  input: 'basic_console.s',
  output: 'basic_console.o',
  command: [nasm, '-f', 'elf32', '-I', meson.current_source_dir() + '/', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

exceptions_o = custom_target('exceptions.o',
  input: 'exceptions.s',
  output: 'exceptions.o',
  command: [nasm, '-f', 'elf32', '-I', meson.current_source_dir() + '/', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

cfuncs_o = custom_target('cfuncs.o',
  input: 'cfuncs.s',
  output: 'cfuncs.o',
  command: [nasm, '-f', 'elf32', '-I', meson.current_source_dir() + '/', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

memops_o = custom_target('memops.o',
  input: 'memops.asm',
  output: 'memops.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

ioports_o = custom_target('ioports.o',
  input: 'ioports.asm',
  output: 'ioports.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

cpuid_o = custom_target('cpuid.o',
  input: 'cpuid.asm',
  output: 'cpuid.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

# Main kernel ELF. We link with gcc but force ldscript + 32-bit.
kernel = executable('test.elf',
  sources: [
    'config.c',
    'stdio_funcs.c',
    'c_exceptions.c',
    'panic.c',
    'inkernel_memory_tests.c',
    'multiboot.c',
    fs_src, # from fs/meson.build
  ],
  include_directories: inc,
  objects: [
    kickoff_o,
    exceptions_o,
    basic_console_o,
    cfuncs_o,
    memops_o,
    ioports_o,
    cpuid_o,
  ],
  link_with: [
    libmmgr,
    lib8259pic,
    libacpi,
    libata_pio,
    libscheduler,
    libps2_controller,
    libearly_serial,
    libnative_api,
    libvolmgr,
    libfs_vfat,
    libprocess,
    libutils,
    libcmos,
    libpci,
    libkb,
  ],
  link_args: [
    '-nostdlib',
    '-static',
    '-Wl,-z,nodefaultlib',
    '-Wl,-T,' + meson.current_source_dir() + '/linker.ld',
    '-Wl,-b,elf32-i386',
  ],
  install: false,
)

# Generate kernel.map (for Bochs) after building the kernel
kernel_map = custom_target('kernel.map',
  input: kernel,
  output: 'kernel.map',
  command: [bash, meson.current_source_dir() + '/gen_bochs_map.sh'],
  build_by_default: true,
)

# Build the standalone test_mmgr program
test_mmgr_exe = executable('test_mmgr',
  sources: [
    'mmgr/test_mmgr.c',
    'test_stubs.c',
    'mmgr/mmgr.c',
  ],
  include_directories: inc,
  c_args: ['-D__BUILDING_TESTS', '-m32'],
  link_args: ['-m32'],
  install: false,
  build_by_default: false,
)
# Enable with: `meson compile -C build test_mmgr`
