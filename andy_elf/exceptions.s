[BITS 32]
section .text

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
global IKeyboard

;Create an IDT (Interrupt Descriptor Table) entry
;The entry is created at ds:esi. esi is incremented to point to the next entry
;destroys values in bl, cx, edi
;edi: offset of handler
;es : GDT selector containing the code
;bl : gate type. 0x05 32-bit task gate, 0x0E 32-bit interrupt gate, 0x0F 32-bit trap gate
;cl : ring level required to call. Only 2 bits are used.
CreateIA32IDTEntry:
	sub edi, 0x7E00		;the passed pointer is the absolute memory address but we need the segment-relative address
	mov word [ds:esi], di	;lower 4 bytes of offset
	mov word [ds:esi+2], es	;selector
	mov byte [ds:esi+4], 0x00	;reserved
	;next byte is (LSB to MSB) gate type x3, reserved 0 x1, DPL x2, present 1 x1
	or bl, 0x80	;set present bit
	clc
	xor ch, ch
	shl cl, 5	;DPL value, shift this to the right position then add it on
	or bl, cl
	mov byte [ds:esi+5], bl
	shr edi, 16
	mov word [ds:esi+6], di
	add esi,8
	ret

;output the given message and hang.
;expects the message pointer in the string table in eax, it applies the string table offset itself. Does not return.
FatalMsg:
	mov esi, eax
	call PMPrintString
	jmp $				;don't return yet

;do-nothing handler for reserved interrupts
IReserved:
	iret

;Trap handlers
IDivZero:
	mov eax, DivZeroMsg
	jmp FatalMsg

IDebug:
	mov eax, DebugMsg
	jmp FatalMsg

INMI:	;technically an interrupt, not a trap
	mov eax, NMIMsg
	jmp FatalMsg

IBreakPoint:
	mov eax, BreakPointMsg
	jmp FatalMsg

IOverflow:
	mov eax, OverflowMsg
	jmp FatalMsg

IBoundRange:
	mov eax, BoundRangeMsg
	jmp FatalMsg

IOpcodeInval:
	mov eax, InvalidOpcodeMsg
	jmp FatalMsg

IDevNotAvail:
	mov eax, DevNotAvailMsg
	jmp FatalMsg

IDoubleFault:	;leaves error code
	pop edx	;move error code into edx
	mov eax, DoubleFaultMsg
	jmp FatalMsg

IInvalidTSS:	;leaves error code
	pop edx
	mov eax, InvalidTSSMsg
	jmp FatalMsg

ISegNotPresent:	;leaves error code
	pop edx
	mov eax, SegmentNotPresentMsg
	jmp FatalMsg

IStackSegFault: ;leaves error code
	pop edx
	mov eax, StackSegmentFaultMsg
	jmp FatalMsg

IGPF:		;leaves error code
	pop edx
	mov eax, GPFMsg
	jmp FatalMsg

IPageFault:	;leaves error code
	pop edx
	mov eax, PageFaultMsg
	jmp FatalMsg

IFloatingPointExcept:
	mov eax, FPExceptMsg
	jmp FatalMsg

IAlignmentCheck:
	pop dx
	mov eax, AlignCheckMsg
	jmp FatalMsg

IMachineCheck:
	mov eax, MachineCheckMsg
	jmp FatalMsg

ISIMDFPExcept:
	mov eax, SIMFPExceptMsg
	jmp FatalMsg

IVirtExcept:
	mov eax, VirtExceptMsg
	jmp FatalMsg

ISecurityExcept:
	mov eax, SecExceptMsg
	jmp FatalMsg

IKeyboard:	;keyboard interrupt handler
	pushf
	push bx
	xor bx, bx
	mov bl, '.'
	call PMPrintChar
	
	;call PIC_Send_EOI
	pop bx
	popf
	iret

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

