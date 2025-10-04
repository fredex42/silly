OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
  . = 0x7E00;

  .text : {
    *(.text)
    *(.text.*)
  }

  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }

  .data : {
    *(.data)
    *(.data.*)
  }

  .bss : {
    *(.bss)
    *(.bss.*)
    *(COMMON)
  }
}
