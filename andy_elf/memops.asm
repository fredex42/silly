[BITS 32]

section .text
global memset
global memcpy
global mb       ;memory barrier
global get_current_paging_directory
global switch_paging_directory_if_required
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

;same as memcopy but copies a number of DWORDS instead of bytes. More efficient for e.g. disk blocks.
memcpy_dw:
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

rep movsd

pop ecx
pop eax
pop es
pop esi
pop edi
pop ebp
ret

;Purpose: returns the current paging directory pointer. This should be stored in CR3.
get_current_paging_directory:
  mov eax, cr3
  ret

;Purpose: switches the paging directory if the current one is not the given one.
; Arguments: the paging directory pointer to check (32-bit)
; Returns: 0 if the paging directory did not change, or the old paging directory value if it did
switch_paging_directory_if_required:
  push ebp
  mov ebp, esp

  mov eax, cr3
  cmp eax, dword [ebp+8]  ;first arg - requested directory pointer
  jz _pg_switch_not_reqd

  ;switch the paging directory over
  push ebx
  mov ebx, dword [ebp+8]
  mov cr3, ebx
  pop ebx
  pop ebp
  ret                       ;eax (return value) is still the old cr3 value

  _pg_switch_not_reqd:
  xor eax,eax               ;return zero
  pop ebp
  ret
