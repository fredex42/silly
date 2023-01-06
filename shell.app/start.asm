[BITS 32]
section .text

global _start

_start:
  mov eax, 0xFF00FF00
  mov edi, testmsg
  int 0x60

  jmp $

section .rodata

testmsg:
db "Test"

section .data

db "Another test"
