[BITS 32]

global exit_to_process

%include "../memlayout.inc"

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
  mov esi, FullTSS
  ;Set the current stack pointer in the TSS. When we return to the kernel we
  ;will effectively have exited from this function.
  mov eax, esp
  mov DWORD [esi+0x04], eax
  ;Get the application stack pointer and store it in a register
  mov ebx, [edi+0x04]
  ;Get hold of the application paging directory physical address and activate it.
  ;This will remove access to the kernel stack.
  mov eax, [edi+0x10]
  mov cr3, eax
  ;set the stack pointer
  mov esp, ebx
  ;FIXME: should restore registers from the application stack
  iret
