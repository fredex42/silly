[BITS 32]

global exit_to_process
global switch_out_process

extern get_current_process  ;defined in process.c  Returns the process struct for the current PID
extern idle_loop            ;defined in kickoss.s. NOT a function, this is our "return address"

%include "../memlayout.asm"

;Purpose: Executes an IRET to exit kernel mode into user-mode on the given process.
;Expects the process stack frame to be configured for the return already; this is either
;done by the call into kernel-mode or by the loader.
;Arguments: 1. Pointer to the struct ProcessTableEntry describing the process.
;Does not return (to the kernel, at least!)
exit_to_process:
  cli

  mov eax, 0x7FFF8                  ;We are exiting the kernel, so re-set the stack to the bottom for when we re-enter
  mov [saved_stack_pointer], eax    ;save the stack pointer to our data area so that we can get it back easily
  
  mov edi, [esp+4]  ;grab the first argument from the stack (pointer to ProcessTableEntry)

  ;Set up registers for the process based on saved state in the process struct
  add edi, 0x28       ;Location of SavedRegisterStates32 within the ProcessTableEntry

  mov eax, [edi+0x44] ;DR7
  mov dr7, eax
  mov eax, [edi+0x40] ;DR6
  mov dr6, eax
  mov eax, [edi+0x3C] ;DR3
  mov dr3, eax
  mov eax, [edi+0x38] ;DR2
  mov dr2, eax
  mov eax, [edi+0x34] ;DR1
  mov dr1, eax
  mov eax, [edi+0x30] ;DR0
  mov dr0, eax
  mov ax, word [edi+0x28] ;GS
  mov gs, ax
  mov ax, word [edi+0x26] ;FS
  mov fs, ax
  mov ax, word [edi+0x24] ;ES
  mov es, ax
  mov ax, word [edi+0x22] ;DS
  mov ds, ax
  mov esi, [edi+0x14] ;ESI
  mov ebp, [edi+0x10] ;EBP
  mov edx, [edi+0x0C] ;EDX
  mov ecx, [edi+0x08] ;ECX
  mov ebx, [edi+0x04] ;EBX

  ;Get hold of the application paging directory physical address and activate it.
  sub edi, 0x28       ;Location of SavedRegisterStates32 within the ProcessTableEntry
  mov eax, [edi+0x0C] ;Location of Page Directory Physical Address within the ProcessTableEntry
  mov cr3, eax

  add edi, 0x28       ;Location of SavedRegisterStates32 within the ProcessTableEntry
  ;Set up a stack frame to return to the user-mode process.  This consists of EIP, CS, EFLAGS, ESP, SS
  mov eax, 0x33      ;FIXME: direct reference to user data seg OR DPL 3
  push eax
  mov eax, [edi+0x48] ;ESP
  push eax
  mov eax, [edi+0x1C] ;EFLAGS
  push eax
  xor eax, eax
  mov ax, word [edi+0x20] ;CS
  push eax
  mov eax, [edi+0x4C] ;EIP
  push eax


  ;now set eax and edi prior to return
  mov eax, [edi+0x00] ;EAX
  mov edi, [edi+0x18] ;EDI
  iret ;go back to process

;Purpose: Saves the current process state into its ProcessTableEntry and switches to kernel context.
; Note: assumes that the preceding stack frame is that of an interrupt handler, so the stack pointer
; at entry points to the saved EIP, CS, EFLAGS, ESP, SS of the interrupted code.
; Arguments: None; pointer to the current process struct is obtained by calling get_current_process
switch_out_process:
  push ebp
  mov ebp, esp

  pushad  ;save all general-purpose registers in this order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
  call get_current_process  ;defined in process.c  Returns the process struct for the current PID in EAX
  test eax, eax
  jz .no_process            ;if no current process, just return
  mov edi, eax

  add edi, 0x28            ;offset of SavedRegisterStates32 in the struct
  mov eax, [ebp-0x20]  ;EAX
  mov [edi + 0x00], eax
  mov eax, [ebp-0x18]  ;EBX
  mov [edi + 0x04], eax
  mov eax, [ebp-0x1C]  ;ECX
  mov [edi + 0x08], eax
  mov eax, [ebp-0x14]  ;EDX
  mov [edi + 0x0C], eax
  mov eax, [ebp]  ;old EBP value
  mov [edi + 0x10], eax
  mov eax, [ebp-0x04]  ;ESI
  mov [edi + 0x14], eax
  mov eax, [ebp-0x00]  ;EDI
  mov [edi + 0x18], eax
  mov eax, [ebp+0x10]  ;EFLAGS from preceding stack frame
  mov [edi + 0x1C], eax
  mov eax, [ebp+0x0C]  ;CS from preceding stack frame
  mov [edi + 0x20], eax
  mov ax, ds
  mov word [edi + 0x22], ax
  mov ax, es
  mov word [edi + 0x24], ax
  mov ax, fs
  mov word [edi + 0x26], ax
  mov ax, gs
  mov word [edi + 0x28], ax
  mov eax, dr0
  mov [edi + 0x30], eax
  mov eax, dr1
  mov [edi + 0x34], eax
  mov eax, dr2
  mov [edi + 0x38], eax
  mov eax, dr3
  mov [edi + 0x3C], eax
  mov eax, dr6
  mov [edi + 0x40], eax
  mov eax, dr7
  mov [edi + 0x44], eax
  mov eax, [ebp+0x14]  ;ESP from the preceding stack frame
  mov [edi + 0x48], eax
  mov eax, [ebp+0x08]  ;EIP from the preceding stack frame
  mov [edi + 0x4C], eax


.no_process:
add esp, 0x20  ;clean up the stack from pushad

pop ebp
pop edi         ;pop the return address from the stack

mov eax, 0x3000 ;FIXME: direct reference to kernel paging directory
mov cr3, eax    ;switch to kernel page directory

mov esp, [saved_stack_pointer] ;restore the kernel stack pointer. This is still necessary because the kernel stack is in a different place to the app stack

push edi         ;push the return address back onto the stack
ret                ;return to the interrupt handler

section .data
saved_stack_pointer dd 0x0
