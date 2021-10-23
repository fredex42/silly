;this file is %-included by andy.sys for static linkage

;output the given message and hang.
;expects the message pointer in the string table in eax, it applies the string table offset itself.	
FatalMsg:
	sub eax, StringTableStart
	add eax, StringTableOffset	;we are actually accessing the string in the data-segment copy. So we need to adjust the offset here.
	mov esi, eax
	call PMPrintString
	jmp $				;don't return yet

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
