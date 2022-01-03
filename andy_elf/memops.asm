[BITS 32]

section .text
global memset
global memcpy
global mb       ;memory barrier
;this module contains basic memset, memcpy implementations.

mb:
  mfence
  ret

;sets the given memory buffer to the provided byte value
;Arguments:
;1. pointer to the buffer to set (expected to be in the current data segment)
;2. byte value to set (uint8_t)
;3. size of the buffer in bytes (uint32_t)
;Returns:
;- pointer to the buffer that was set.
memset:
  push ebp
  mov ebp, esp ;store the stack pointer so we can access arguments
  push edi
  push es
  push eax
  push ecx

  ;stosb stores at the location pointed by es:edi
  mov ax, ds
  mov es, ax
  mov edi, [ebp+8]  ;first arg - the buffer to set

  mov al, byte [ebp+12]  ;second arg - the character to apply
  mov ecx, dword [ebp+16] ; third arg - byte count to copy

  rep stosb

  pop ecx
  pop eax
  pop es
  pop edi
  pop ebp
  ret

;copy data from one memory buffer to another
;Arguments:
;1. pointer to the destination buffer
;2. pointer to the source buffer
;3. size of the buffer in bytes (uint32_t)
;Returns:
;- pointer to the destination buffer
memcpy:
  push ebp
  mov ebp, esp
  push edi
  push esi
  push es
  push eax
  push ecx

  xor ecx, ecx
  mov cx, ds
  mov es, cx
  mov edi, [ebp+8]  ;first arg - destination buffer
  mov esi, [ebp+12]  ;second arg - source buffer
  mov ecx, dword [ebp+16]

  rep movsb

  pop ecx
  pop eax
  pop es
  pop esi
  pop edi
  pop ebp
  ret
