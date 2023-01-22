[BITS 32]

section .text
global init_native_api
global native_api_landing_pad

%include "apicodes.inc"
%include "../memlayout.inc"
extern CreateIA32IDTEntry

;process_ops.c
extern api_terminate_current_process
extern api_sleep_current_process
extern api_create_process

;stream_ops.c
extern api_close
extern api_open
extern api_read
extern api_write

;scheduler/lowlevel.asm
extern enter_kernel_context

;Purpose - initialise the native API by attaching the landing pad function to
; the int 0x60 interrupt
init_native_api:
  push ebp
  mov ebp, esp

  push es
  mov ax, 0x08      ;kernel CS
  mov es, ax
  mov esi, IDTOffset
  add esi, 0x300  ;jump forward 0x60 (=96) entries of 8 bytes each
  mov edi, native_api_landing_pad
  mov bl, 0x0E    ;interrupt gate
  mov cx, 0x0003

  call CreateIA32IDTEntry

  pop es
  pop ebp
  ret

;this interrupt handler is called for every native API call. Its job is to dispatch
;the call into the necessary handler (usually a C function)
native_api_landing_pad:
  call enter_kernel_context     ;this should switch to kernel context and then land here. FIXME should not restore registers!
  cmp eax, API_EXIT
  jnz .napi_2
  call api_terminate_current_process ;does not return
  jmp .napi_rtn
.napi_2:
  cmp eax, API_SLEEP
  jnz .napi_3
  call api_sleep_current_process
  jmp .napi_rtn
.napi_3:
  cmp eax, API_CREATE_PROCESS
  jnz .napi_4
  call api_create_process
  jmp .napi_rtn
.napi_4:
  cmp eax, API_CLOSE
  jnz .napi_5
  call api_close
  jmp .napi_rtn
.napi_5:
  cmp eax, API_OPEN
  jnz .napi_6
  call api_open
  jmp .napi_rtn
.napi_6:
  cmp eax, API_READ
  jnz .napi_7
  call api_read
  jmp .napi_rtn
.napi_7:
  cmp eax, API_WRITE
  jnz .napi_8
  push ecx          ;length
  push esi          ;pointer
  and ebx, 0xFFFF
  push ebx          ;file descriptor
  call api_write
  add esp, 12

  jmp .napi_rtn
.napi_8:
  ;we did not recognise the API code. Fallthrough to return to process.
  mov eax, API_ERR_NOTFOUND

.napi_rtn:
  iret
