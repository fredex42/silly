[BITS 32]
section .text

global _start

_start:
  mov eax, 0x0000000B ;"API write()"
  mov bx,  0x01       ;file descriptor
  mov ecx, 19         ;string length
  mov esi, testmsg    ;string pointer
  int 0x60

  jmp _start

section .rodata

testmsg:
db 'Hello from ring 3', 0x0D, 0x0A, 0x00

section .data

db "Another test"
