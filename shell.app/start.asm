[BITS 32]
section .text

global _start

_start:
  mov eax, 0x0000000B ;"API write()"
  mov bx,  0x01       ;file descriptor
  mov ecx, 20         ;string length
  mov esi, testmsg    ;string pointer
  int 0x60

  ;cli     ;this should cause a GPF

  jmp _start

section .rodata

testmsg:
db 'Hello from ring 3', 0x0A, 0x0D, 0x00

section .data

db "Another test"
