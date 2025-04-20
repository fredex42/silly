[BITS 32]

section .text
;this defines 'bridge' functions which are callable from C
;we expect a calling convention where the arguments are pushed onto the stack
;so the stack frame will be: [sp..sp+4] saved bp value [sp+4..sp+8] return addr [sp+8..sp+12] arg1 [sp+12..sp+16] arg2 .....
global kputs
global kputlen
%include "basic_console.inc"

;Purpose: outputs a string to the console
;Expects: 32-bit pointer to a string (in kernel DS) on the stack, behind the return address
;Returns: Nothing
kputs:
	push ebp
	mov ebp, esp			;save the stack pointer where we started
	push ds

	push esi
	push ebx
	mov esi, dword [ss:ebp+8]	;get the pointer to the string to print (arg1 is offset 8). This will be an absolute address,
					;so we resolve it relative to DS which starts at 0
	call PMPrintString
	pop ebx
	pop esi
	pop ds
	
	; This is an inline delay loop, used to slow text on real hardware in order to make it readable.
	; push ecx
	; mov ecx, 0xffffff
	; .kp_delay_temp:
	; dec ecx
	; test ecx, ecx
	; jnz .kp_delay_temp
	; pop ecx
	
	pop ebp
	ret

;Purpose: outputs a string to the console
;Expects: 32-bit pointer to a string (in kernel DS) on the stack, followed by an unsigned 32-bit integer representing the length
;Returns: Nothing
kputlen:
	push ebp
	mov ebp, esp

	push esi
	push ecx
	push ebx
	mov esi, dword [ss:ebp+8] ;get the pointer to the string to print (arg1 is offset 8). This will be an absolute address,
					;so we resolve it relative to DS which starts at 0
	mov ecx, dword [ss:ebp+12]	;get the length value from the next argument
	call PMPrintStringLen

	; This is an inline delay loop, used to slow text on real hardware in order to make it readable.
	; mov ecx, 0xffffff
	; .kpl_delay_temp:
	; dec ecx
	; test ecx, ecx
	; jnz .kpl_delay_temp

	pop ebx
	pop ecx
	pop esi

	pop ebp
	ret
