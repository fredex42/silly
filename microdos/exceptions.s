[BITS 32]
section .text

;this function defines very basic handlers for standard intel exceptions,
;which consist of printing a diagnostic message to the screen and then locking.

%include "basic_console.inc"
;function exports
global CreateIA32IDTEntry
global IDivZero
global IDebug
global INMI
global IBreakPoint
global IOverflow
global IBoundRange
global IOpcodeInval
global IDevNotAvail
global IDoubleFault
global IReserved
global IInvalidTSS
global ISegNotPresent
global IStackSegFault
global IGPF
global IPageFault
global IFloatingPointExcept
global IAlignmentCheck
global IMachineCheck
global ISIMDFPExcept
global IVirtExcept
global ISecurityExcept

;function imports
extern c_except_div0
extern c_except_gpf
extern c_except_invalidop
extern c_except_pagefault

;output the given message and hang.
;expects the message pointer in the string table in eax, it applies the string table offset itself. Does not return.
FatalMsg:
	mov esi, eax
	call PMPrintString
	nop
	nop
	jmp $				;don't return yet

;do-nothing handler for reserved interrupts
IReserved:
	iret

;Trap handlers
IDivZero:
	mov ax, 0x10
	mov ds, ax
	call c_except_div0	;helpfully, the CPU has already arranged a stack frame for us that is compatible with GCC
	mov eax, DivZeroMsg
	call FatalMsg

IDebug:
	push ds
	push es
	push esi
	push eax
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov esi, DebugMsg
	call PMPrintString
	pop eax
	pop esi
	pop es
	pop ds
	iret

INMI:	;technically an interrupt, not a trap
	mov ax, 0x10
	mov ds, ax
	mov eax, NMIMsg
	call FatalMsg

IBreakPoint:
	mov ax, 0x10
	mov ds, ax
	mov eax, BreakPointMsg
	call FatalMsg

IOverflow:
	mov ax, 0x10
	mov ds, ax
	mov eax, OverflowMsg
	call FatalMsg

IBoundRange:
	mov ax, 0x10
	mov ds, ax
	mov eax, BoundRangeMsg
	call FatalMsg

IOpcodeInval:
	mov ax, 0x10
	mov ds, ax
  call c_except_invalidop
	mov eax, InvalidOpcodeMsg
	call FatalMsg

IDevNotAvail:
	mov ax, 0x10
	mov ds, ax
	mov eax, DevNotAvailMsg
	call FatalMsg

IDoubleFault:	;leaves error code
	mov ax, 0x10
	mov ds, ax
	pop edx	;move error code into edx
	mov eax, DoubleFaultMsg
	call FatalMsg

IInvalidTSS:	;leaves error code
	mov ax, 0x10
	mov ds, ax
	pop edx
	mov eax, InvalidTSSMsg
	call FatalMsg

ISegNotPresent:	;leaves error code
	mov ax, 0x10
	mov ds, ax
	pop edx
	mov eax, SegmentNotPresentMsg
	call FatalMsg

IStackSegFault: ;leaves error code
	mov ax, 0x10
	mov ds, ax
	pop edx
	mov eax, StackSegmentFaultMsg
	call FatalMsg

IGPF:		;leaves error code
	mov ax, 0x10
	mov ds, ax
	call c_except_gpf
	mov eax, GPFMsg
	call FatalMsg

IPageFault:	;leaves error code
	;We shouldn't mess with the stack, because c_except_pagefault is relying on the contents.
	;But, we must ensure that we restore registers.... how to fix????
	;Thought - best to duplicate up the stack frame
	push ebp
	mov ebp, esp		;save start of stack frame as we came in (+4 bytes)

	push eax			;Any of these _could_ be messed around by the C code
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ds
	push es
	push fs
	push gs

	mov ax, 0x10
	mov ds, ax

	;When we entered, our stack frame looked like this: uint32_t error_code, uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags
	;we need to add the faulting address
	;but since we have messed with the stack already we must recover the previous values and copy them to the top of the stack
	mov eax, dword [ebp+16]	;eflags
	push eax
	mov eax, dword [ebp+12]	;faulting_codeseg
	push eax
	mov eax, dword [ebp+8]	;faulting_addr
	push eax
	mov eax, dword [ebp+4]	;error_code
	push eax
	mov eax, cr2				;pf_load_addr
	push eax
	call c_except_pagefault
	cmp eax, 0
	jz .recovered

	mov eax, PageFaultMsg
	call FatalMsg
	.recovered:
	add esp, 20					;reset stack-frame to before the C call
	
	;now restore registers
	pop gs
	pop fs
	pop es
	pop ds
	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	add esp, 4					;the topmost value coming in was the error_code, we must drop this so iret has a valid stack-frame to return
	iret
	
IFloatingPointExcept:
	mov eax, FPExceptMsg
	call FatalMsg

IAlignmentCheck:
	pop dx
	mov eax, AlignCheckMsg
	call FatalMsg

IMachineCheck:
	mov eax, MachineCheckMsg
	call FatalMsg

ISIMDFPExcept:
	mov eax, SIMFPExceptMsg
	call FatalMsg

IVirtExcept:
	mov eax, VirtExceptMsg
	call FatalMsg

ISecurityExcept:
	mov eax, SecExceptMsg
	call FatalMsg

section .data
;all these errors are fatal if the occur in the kernel. see https://wiki.osdev.org/Exceptions
DivZeroMsg db 'FATAL: Divide by zero', 0
DebugMsg db 'FATAL: Debug trap', 0
NMIMsg db 'FATAL: Non-maskable interrupt occurred', 0
BreakPointMsg db 'FATAL: Hardware breakpoint', 0
OverflowMsg db 'FATAL: An overflow error occurred', 0
BoundRangeMsg db 'FATAL: Bound range exceeded', 0
InvalidOpcodeMsg db 'FATAL: Invalid opcode', 0
DevNotAvailMsg db 'FATAL: Device not availalbe', 0
DoubleFaultMsg db 'FATAL: Double fault', 0	;leaves error code on top of stack
InvalidTSSMsg db 'FATAL: Invalid TSS', 0	;leaves error code
SegmentNotPresentMsg db 'FATAL: Segment not present', 0	;leaves error code
StackSegmentFaultMsg db 'FATAL: Stack segment fault', 0	;leaves error code
GPFMsg db 'FATAL: General protection fault', 0		;leaves error code
PageFaultMsg db 'FATAL: Page fault', 0			;leaves error code
FPExceptMsg db 'FATAL: Floating-point Exception', 0
AlignCheckMsg db 'FATAL: Alignment check failed', 0	;leaves error code
MachineCheckMsg db 'FATAL: Machine check failed', 0
SIMFPExceptMsg db 'FATAL: SIMD floating-point exception', 0
VirtExceptMsg db 'FATAL: Virtualization exception', 0
SecExceptMsg db 'FATAL: Security exception', 0		;leaves error code
