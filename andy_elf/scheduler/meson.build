lowlevel_o = custom_target('sched_lowlevel.o',
  input: 'lowlevel.asm',
  output: 'sched_lowlevel.o',
  command: [nasm, '-f', 'elf32', '@INPUT@', '-o', '@OUTPUT@'],
  build_by_default: true,
)

libscheduler = static_library('scheduler',
  sources: [
    'scheduler_task.c',
    'scheduler.c',
  ],
  objects: [lowlevel_o],
  include_directories: inc,
)
