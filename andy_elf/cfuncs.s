[BITS 32]

section .text
;this defines 'bridge' functions which are callable from C
;we expect a calling convention where the arguments are pushed onto the stack
;so the stack frame will be: [sp..sp+4] saved bp value [sp+4..sp+8] return addr [sp+8..sp+12] arg1 [sp+12..sp+16] arg2 .....
global kputs
global kputlen
global longToString
global __stack_chk_fail
%include "basic_console.inc"

;Purpose: outputs a string to the console
;Expects: 32-bit pointer to a string (in kernel DS) on the stack, behind the return address
;Returns: Nothing
kputs:
	push ebp
	mov ebp, esp			;save the stack pointer where we started
	push ds

	push esi
	mov esi, dword [ss:ebp+8]	;get the pointer to the string to print (arg1 is offset 8). This will be an absolute address,
					;so we resolve it relative to DS which starts at 0
	call PMPrintString
	pop esi

	pop ds
	mov esp, ebp
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
	mov esi, dword [ss:ebp+8] ;get the pointer to the string to print (arg1 is offset 8). This will be an absolute address,
					;so we resolve it relative to DS which starts at 0
	mov ecx, dword [ss:ebp+12]	;get the length value from the next argument
	call PMPrintStringLen

	pop ecx
	pop esi
	mov esp, ebp
	pop ebp
	ret

;Purpose: converts an integer into a string (in base 10). Preserves registers but not flags.
;Expects: 32-bit integer representing the number to convert on the stack, behind the return address
;	  32-bit pointer giving the location of an allocated string to push the result into
;   32-bit number giving the base to convert with
;See: https://runestone.academy/runestone/books/published/pythonds/Recursion/pythondsConvertinganIntegertoaStringinAnyBase.html
longToString:
	push ebp
	mov ebp, esp			;save the stack pointer where we started

	push edx
	push eax
	push ecx
	push ebx
	push edi
	push esi

	mov esi, esp			;stack pointer where we start operation
	mov eax, dword [ss:ebp+8]	;set eax to the number to be converted
	xor bh,bh

	mov ecx, dword [ss:ebp+16]	;number base
	test ecx,ecx
	jz longToStringEnd					;don't try to divide by zero

	div_loop:
	xor edx, edx	;edx is the upper 32 bits of the numerator going in and the remained going out. Ensure it's 0 before we start
	div ecx				;divide by the number base. Result is in EAX and remainder is in EDX. Remainder is our numeral
	mov bl, byte [edx+numerals]	;set bl to the ascii code of the numeral
	push bx				;push that result onto the stack
	cmp eax, dword [ss:ebp+16]	;keep looping until out result is < the number base
	jge div_loop

	;last digit is in EAX
	mov bl, byte [eax+numerals]
	push bx

	;now we have the characters pushed onto the stack, move them into the provided buffer
	mov edi, dword [ss:ebp+12]	;set edi to the location
	store_loop:
	pop ax				;pop the next character off the stack
	mov byte [edi], al		;store it in the string
	inc edi
	cmp esp, esi			;loop until our stack is at the same point as where we started the operation
	jnz store_loop

  mov byte [edi], 0		;null-terminate

	longToStringEnd:
	pop esi
	pop edi
	pop ebx
	pop ecx
	pop eax
	pop edx
	mov esp, ebp
	pop ebp
	ret

;Purpose: report a stack overflow from C code. Required for gcc.
__stack_chk_fail:
	mov esi, overflow
	call PMPrintString
	jmp $

section .data
numerals: db '0123456789abcdef'
overflow: db 'PANIC - kernel stack overflow\r\n'
