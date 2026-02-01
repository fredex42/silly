[BITS 32]

section .text
global init_native_api
global native_api_landing_pad

%include "../memlayout.asm"
extern CreateIA32IDTEntry

;kickoff.asm
extern idle_loop


;scheduler/lowlevel.asm
extern switch_out_process

; ;drivers/cmos/rtc.c
; extern rtc_get_epoch_time
extern native_api_dispatcher


;this interrupt handler is called for every native API call. Its job is to dispatch
;the call into the necessary handler (usually a C function)
native_api_landing_pad:
  ; eax = api code
  ; ebx, ecx, edx, esi, edi = api parameters
  push ebp
  push edi
  push esi
  push edx
  push ecx
  push ebx
  push eax

  call native_api_dispatcher
  ;return value in eax

  ;FIXME - need to check new scheduler state to detect if we need to switch processes into kernel runloop
  add esp, 16
  pop esi
  pop edi
  pop ebp
  iret

temp_label:
.napi_nf:
  ;we did not recognise the API code. Fallthrough to return to process.
  mov eax, API_ERR_NOTFOUND

.napi_rtn_direct:
  iret

.napi_rtn_to_kern:
  call switch_out_process
  ;set up a stack frame that gets us back to the kernel idle loop
  pushf
  xor eax, eax
  mov eax, cs
  push eax
  mov eax, idle_loop
  push eax
  iret
