[BITS 32]

section .text

global strncmp
global strncpy

;Purpose - copies the ASCIIZ string `src` to the ASCIIZ string `dest`, up to a maximum
;of `len` characters. Ensures that `dest` is zero-terminated.
;It's assumed that both are in the data segment.
; clobbers ax, cx, dx, di, si
strncpy:
  push ebp
  mov ebp, esp

  push es
  mov ax, ds
  mov es, ax

  mov edi, dword [ss:ebp+8]  ;ARG 1 - `dest`
  mov esi, dword [ss:ebp+12] ;ARG 2 - `src`
  mov ecx, dword [ss:ebp+16] ;ARG 3 - `len`

  xor edx,edx
  xor eax, eax
  _strncpy_next_byte:
  test edx, ecx
  jz _strncpy_done  ;we ran out of len

  lodsb
  test al,al
  jz _strncpy_done  ;we reached the end of the source string

  stosb
  inc edx
  jmp _strncpy_next_byte

  _strncpy_done:
  xor al,al
  stosb           ;ensure we store a terminating null

  pop es
  pop ebp
  mov eax, edx    ;return value - the number of characters copied
  ret

;Purpose - compares the ASCIIZ string `A` to the ASCIIZ string `B`, up to a maximum of
;`len` characters.
;It's assumed that both are in the data segment.
strncmp:
  push ebp
  mov ebp, esp

  mov edi, dword [ss:ebp+8]  ;ARG 1 - A
  mov esi, dword [ss:ebp+12] ;ARG 2 - B
  mov ecx, dword [ss:ebp+16] ;ARG 3 - `len`

  xor eax, eax
  _strncmp_next_byte:
  test ecx, ecx
  jz _strncmp_done  ;we ran out of len

  lodsb ;loads byte from ds:si into AL
  sub al, byte [edi]

  test al, al
  jnz _strncmp_done ;we found a difference

  inc edi
  dec ecx
  jmp _strncmp_next_byte

  _strncmp_done:
  ; OK, so if AL=0 we have identical, Otherwise it's a difference
  pop ebp
  ret
