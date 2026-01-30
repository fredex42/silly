; crt0.asm
global _start
extern main
extern _exit

section .text
_start:
      call main
      call _exit
