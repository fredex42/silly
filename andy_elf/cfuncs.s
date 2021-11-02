[BITS 32]

;this defines 'bridge' functions which are callable from C
;we expect a calling convention where the arguments are pushed onto the stack
;so the stack frame will be: [sp..sp+4] saved bp value [sp+4..sp+8] return addr [sp+8..sp+12] arg1 [sp+12..sp+16] arg2 .....
global kputs

%include "basic_console.inc"

;Purpose: outputs a string to the console
;Expects: 32-bit pointer to a string (in kernel DS) on the stack, behind the return address
;Returns: Nothing
kputs:
	push ebp
	mov ebp, esp		;ebp should now point to the lowest argument
	push ds

	push esi
	mov esi, dword [ss:ebp+8]	;get the pointer to the string to print (arg1 is offset 12). This will be an absolute address,
					;so we resolve it relative to DS which starts at 0
	call PMPrintString
	pop esi
	
	pop ds
	mov esp, ebp
	pop ebp
	ret
