;this file is %-included by andy.sys for static linkage

;output the given message and hang.
;expects the message pointer in the string table in eax, it applies the string table offset itself. Does not return.
FatalMsg:
	sub eax, StringTableStart
	add eax, StringTableOffset	;we are actually accessing the string in the data-segment copy. So we need to adjust the offset here.
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
