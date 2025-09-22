[BITS 32]

global exit_to_process
global enter_kernel_context

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

;Purpose: Switches to kernel context. NOTE: this will return to the function that it
;was called from BUT will not preserve the stack below it. Therefore it must be called at
;the outer level immediately on entering an interrupt not from an arbitary C function
;It is also essential that interrupts are disabled when calling this because it could leave
;the CPU in an undefined state (resulting in a triple-fault) if interrupted
enter_kernel_context:
  ;first we need to save the register states, BUT we don't actually know where the process struct is right now.
  ;And we can't find it without clobbering registers.
  ;So, we first save the regs into a scratch area and then copy that into the process struct
  ;the scratch area lives at a fixed memory location given in memlayout.asm
  push edi
  mov edi, CurrentProcessRegisterScratch
  mov [edi + 0x00], eax
  mov [edi + 0x04], ebx
  mov [edi + 0x08], ecx
  mov [edi + 0x0C], edx
  mov [edi + 0x10], ebp
  mov [edi + 0x14], esi
  ;we are using EDI, so put its former value into EAX and commit it from there
  pop eax
  mov [edi + 0x18], eax   ;actually, inital value of EDI
  pushf
  pop eax
  mov [edi + 0x1C], eax   ;actually the value of EFLAGS

  xor eax, eax
  mov ax, cs
  mov [edi + 0x20], ax
  mov ax, ds
  mov [edi + 0x22], ax
  mov ax, es
  mov [edi + 0x24], ax
  mov ax, fs
  mov [edi + 0x26], ax
  mov ax, gs
  mov [edi + 0x28], ax
  xor eax, eax
  mov [edi + 0x2A], eax
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
  mov eax, esp              ;this is the current stack pointer (points at return address from the CALL)
  ;The CPU pushed EFLAGS, CS, EIP when the interrupt happened, then CALL pushed a return address.
  ;we PUSHed EDI above, but popped it back out into EAX above
  ;After that POP the stack points at the return address, so add 4 to get back to the interrupted EIP.
  add eax, 0x04
  mov [edi + 0x48], eax     ;process stack pointer (points at EIP) when we entered the interrupt handler

  ; OK, now we have saved the registers switch into kernel context

  mov ax, 0x10  ;FIXME direct reference to kernel data seg
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  pop edi            ;grab the return address from the stack
  mov eax,  0x3000  ;FIXME direct reference to kernel paging directory location as defined in mmgr.c
  mov cr3, eax
  ;NOTE stack pointer is invalid at this point
  mov esp, [saved_stack_pointer]  ;this was set when we exited kernel mode last
  push edi           ;put the return address down onto the stack pointer

  ;Now we are in kernel context, copy the register state over to the process table
  call get_current_process    ;eax should hold the address of the process struct. C function so can't rely on registers now.
  mov edi, eax
  add edi, 0x24               ;offset of SavedRegisterStates32 in the struct (destination pointer)
  mov esi, CurrentProcessRegisterScratch  ;source pointer
  mov ecx, 0x13               ; 0x4c bytes = 0x13 dwords
  rep movsd                   ; copy the data across

  ;we've messed around with some of the registers, so restore their values to what they were when we entered
  mov esi, CurrentProcessRegisterScratch 
  mov eax, [esi + 0x00]   ;restore the value of eax which was present before
  mov ecx, [esi + 0x08]
  mov edi, [esi + 0x18]
  mov esi, [esi + 0x14]

  ret                ;return to the interrupt handler

section .data
saved_stack_pointer dd 0x0
