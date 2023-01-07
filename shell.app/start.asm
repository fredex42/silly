[BITS 32]
section .text

global _start

_start:
  mov eax, 0x0000000B ;"API write()"
  mov bx,  0x01       ;file descriptor
  mov ecx, 19         ;string length
  mov esi, testmsg    ;string pointer
  int 0x60

  jmp $

section .rodata

testmsg:
db "Hello from ring 3\r\n"

section .data

db "Another test"
