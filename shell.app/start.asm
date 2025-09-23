[BITS 32]
section .text

global _start

_start:
  mov eax, 0x0000000B ;"API write()"
  mov bx,  0x01       ;file descriptor (stdout)
  mov ecx, 20         ;string length
  mov esi, testmsg    ;string pointer
  int 0x60

  mov eax, 0x0B
  mov bx, 0x01
  mov ecx, 23
  mov esi, waiting
  int 0x60

  .loop_wait_key:
  mov eax, 0x0A       ;"API read()"
  mov bx, 0x00        ;file descriptor (stdin)
  mov ecx, 0x01       ;buffer length
  mov esi, buf        ;pointer to buffer
  int 0x60

  
  cmp byte [buf], 'q' ;if 'q' was pressed, exit
  jne .loop_wait_key

  ;cli     ;this should cause a GPF

  mov eax, 0x1        ;"API exit()"
  int 0x60

  jmp _start

section .rodata

testmsg:
db 'Hello from ring 3', 0x0A, 0x0D, 0x00
waiting:
db 'Waiting for keyboard...', 0x0A, 0x0D, 0x00
buf:
db 0x00

section .data

db "Another test"
