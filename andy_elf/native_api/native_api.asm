[BITS 32]

section .text
global init_native_api
global native_api_landing_pad

%include "apicodes.asm"
%include "../memlayout.asm"
extern CreateIA32IDTEntry

;kickoff.asm
extern idle_loop

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

;drivers/cmos/rtc.c
extern rtc_get_epoch_time

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

; native_api_landing_pad_v2:
;   call enter_kernel_context
;   mov esi, .napi_jump_table
;   add esi, eax
;   cmp esi, .napi_jump_table_end
;   jg .napi_nf
;   call esi
;   jmp .napi_rtn_to_kern

; .napi_jump_table:
;   dd api_terminate_current_process
;   dd api_sleep_current_process
;   dd api_create_process
;   dd api_close
;   dd api_open
;   dd api_read
;   dd api_write
;   dd api_get_time
; .napi_jump_table_end:
;   nop

;this interrupt handler is called for every native API call. Its job is to dispatch
;the call into the necessary handler (usually a C function)
native_api_landing_pad:
  cmp eax, API_EXIT
  jnz .napi_2
  call api_terminate_current_process ;puts the process record into a TERMINATING state. The scheduler will trigger cleanup
  call enter_kernel_context          ;exit back to the kernel idle loop
  jmp .napi_rtn_to_kern
.napi_2:
  cmp eax, API_SLEEP
  jnz .napi_3
  call enter_kernel_context     ;this should switch to kernel context and then land here. FIXME should not restore registers!
  call api_sleep_current_process
  jmp .napi_rtn_to_kern
.napi_3:
  cmp eax, API_CREATE_PROCESS
  jnz .napi_4
  call enter_kernel_context     ;this should switch to kernel context and then land here. FIXME should not restore registers!
  call api_create_process
  jmp .napi_rtn_to_kern
.napi_4:
  cmp eax, API_CLOSE
  jnz .napi_5
  call api_close
  jmp .napi_rtn_direct
.napi_5:
  cmp eax, API_OPEN
  jnz .napi_6
  call api_open
  jmp .napi_rtn_direct
.napi_6:
  cmp eax, API_READ
  jnz .napi_7
  call api_read
  jmp .napi_rtn_direct
.napi_7:
  cmp eax, API_WRITE
  jnz .napi_8
  push ecx          ;length
  push esi          ;pointer
  and ebx, 0xFFFF
  push ebx          ;file descriptor
  call api_write
  add esp, 12

  jmp .napi_rtn_direct
.napi_8:
  cmp eax, API_GET_TIME
  jnz .napi_nf
  push ebx
  push ecx
  push edx
  push edi
  push esi
  call rtc_get_epoch_time
  pop esi
  pop edi
  pop edx
  pop ecx
  pop ebx
  jmp .napi_rtn_direct

.napi_nf:
  ;we did not recognise the API code. Fallthrough to return to process.
  mov eax, API_ERR_NOTFOUND

.napi_rtn_direct:
  iret

.napi_rtn_to_kern:
  ;set up a stack frame that gets us back to the kernel idle loop
  pushf
  xor eax, eax
  mov eax, cs
  push eax
  mov eax, idle_loop
  push eax
  iret
