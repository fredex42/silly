[BITS 32]

section .text
global memset
global memset_dw
global memcpy
global memcpy_dw
global memcmp
global mb       ;memory barrier
global get_current_paging_directory
global switch_paging_directory_if_required
;this module contains basic memset, memcpy implementations.

mb:
  ;mfence is only for SSE2-capable CPUs, so we use a more basic method
  lock add dword [esp], 0
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

;sets the given memory buffer to the provided DWORD value
;Arguments:
;1. pointer to the buffer to set (in current DS)
;2. dword value to set (uint32_t)
;3. size of the buffer in dwords (uint32_t). NOTE that this is 1/4 of the byte length!
;Returns:
;- pointer to the buffer that was set
memset_dw:
  push ebp
  mov ebp, esp
  push edi
  push es
  push ecx

  mov ax, ds
  mov es, ax
  mov edi, [ebp+8]    ;first arg - buffer

  mov eax, dword [ebp+12] ;second arg - value to set
  mov ecx, dword [ebp+16] ;third arg  - dword count to copy

  rep stosd

  pop ecx
  pop es
  mov eax, edi
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

;Purpose - compares two memory ranges byte-by-byte
; Arguments:
;  1. Memory range A pointer
;  2. Memory range B pointer
;  3. Length of memory range to compare
; Returns 0 if they are equal, 1 if B is greater and -1 if B is less
memcmp:
  push ebp
  mov ebp, esp

  push edi
  push esi
  push ebx
  push ecx

  mov edi, dword [ebp+8]    ;Argument 1 - memory range A
  mov esi, dword [ebp+12]   ;Argument 2 - memory range B
  mov ecx, dword [ebp+16]   ;Argument 3 - length

  .memcmp_next:
  mov al, byte [edi]
  mov bl, byte [esi]
  cmp al, bl
  jg .memcmp_greater_than
  jl .memcmp_less_than
  dec ecx
  test ecx, ecx
  jz .memcmp_zero
  inc edi
  inc esi
  jmp .memcmp_next

  .memcmp_greater_than:
  mov eax, 1
  jmp .memcmp_done

  .memcmp_less_than:
  mov eax, 0xFFFFFFFF
  jmp .memcmp_done

  .memcmp_zero:
  mov eax, 0

  .memcmp_done:
  pop ecx
  pop ebx
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
