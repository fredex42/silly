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
  xor eax, eax

  mov ax, 0x33      ;FIXME: direct reference to user data seg OR DPL 3
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  mov edi, [esp+4]  ;grab the first argument from the stack
  ;Set the current stack pointer in the TSS TO THE APPLICATION STACK. When we return to the kernel we
  ;will effectively have exited from this function. in the stack context of the application.
  ;Get the application stack pointer and store it in a register
  mov esi, edi
  add esi, 0x28       ;Location of SavedRegisterStates32 within the ProtessTableEntry
  mov ebx, [esi+0x48] ;location of ESP in SavedRegisterStates32


  ;Set up a stack frame so we can return to the kernel easily after an int call. CPU expects to pop EIP, CS, EFLAGS in that order
  pushf
  mov ax, cs
  push eax
  mov eax, idle_loop ;idle_loop is a label defined in kickoff.s and it's where we want to get back to
  push eax

  mov eax, 0x7FFF8                      ;We are exiting the kernel, so re-set the stack to the bottom for when we re-enter
  mov [saved_stack_pointer], eax    ;save the stack pointer to our data area so that we can get it back easily

  mov esi, FullTSS
  mov DWORD [esi+0x04], ebx

  ;Get hold of the application paging directory physical address and activate it.
  ;This will remove access to the kernel stack.
  mov eax, [edi+0x0C]
  mov cr3, eax
  ;set the stack pointer
  mov esp, ebx

  ;restore the register states from the process struct
  ;first, deal with EFLAGS
  mov eax, [esi+0x1C]
  push eax
  popf
  ;now get the ESI value we want and put that on the stack for when we have finished
  mov eax, [esi+0x14]
  push eax

  ;ignore segment selector registers as there is no other valid value for them to have

  ;debug registers
  mov eax, [esi+0x30]
  mov dr0, eax
  mov eax, [esi+0x34]
  mov dr1, eax
  mov eax, [esi+0x38]
  mov dr2, eax
  mov eax, [esi+0x3C]
  mov dr3, eax
  mov eax, [esi+0x40]
  mov dr6, eax
  mov eax, [esi+0x44]
  mov dr7, eax

  ;general-use registers
  mov eax, [esi+0x00]
  mov ebx, [esi+0x04]
  mov ecx, [esi+0x08]
  mov edx, [esi+0x0C]
  mov ebp, [esi+0x10]
  ;can't set esi yet, come back to it!
  mov edi, [esi+0x18]


  ;finally, restore esi from the stack
  pop esi
  iret

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
  push edi
  call dump_process_struct
  add esp, 4

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
extern dump_process_struct
sub edi, 0x28            ;offset of SavedRegisterStates32 in the struct
push edi
call dump_process_struct
add esp, 4

pop ebp
pop edi         ;pop the return address from the stack

mov eax, 0x3000 ;FIXME: direct reference to kernel paging directory
mov cr3, eax    ;switch to kernel page directory

mov esp, [saved_stack_pointer] ;restore the kernel stack pointer. This is still necessary because the kernel stack is in a different place to the app stack

push edi         ;push the return address back onto the stack
ret                ;return to the interrupt handler

section .data
saved_stack_pointer dd 0x0
