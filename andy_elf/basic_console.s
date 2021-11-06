[BITS 32]
section .text
;this module contains a couple of very simple assembly routines
;for putting text strings onto the console in standard text mode
;it works by direct manipulation of the framebuffer at 0xB800
global PMPrintString
global PMPrintChar
global PMPrintStringLen

%include "memlayout.inc"

;Print an ASCIIZ string in protected mode.
;Expects the string to print in ds:esi
PMPrintString:
	push es
	push edi
	push eax
	push ecx
	push edx
	pushf

	;set up es:edi to point to where we want to write the characters
	mov ax, DisplayMemorySeg
	mov es, ax
	mov edi, TextConsoleOffset

	;calculate the correct display offset based on the cursor position
	xor eax, eax
	mov al, byte [CursorRowPtr]	;assume 80 columns per row = 0x50. Double this to 0xa0 because each char is 2 bytes (char, attribute)
	mov cx, 0x00a0
	mul cx
	xor dx, dx
	mov dl, byte [CursorColPtr]
	add ax, dx			;AX is the offset
	add ax, dx			;add again to double the offset
	add edi, eax

	pm_next_char:
	lodsb
	test al, al
	jz pm_string_done	;if the next char is 0, then exit
	cmp al, 0x0d		;CR
	jz pm_carraige_rtn
	cmp al, 0x0a		;LF
	jz pm_linefeed

	stosb
	mov al, DefaultTextAttribute
	stosb	;only use default attribute at the moment

	inc byte [CursorColPtr]
	cmp byte [CursorColPtr], 0x50	;columns-per-row
	jnz pm_next_char
	mov byte [CursorColPtr], 0
	inc byte [CursorRowPtr]
	jmp pm_next_char

	pm_carraige_rtn:
	mov byte [CursorColPtr], 0
	jmp pm_next_char

	pm_linefeed:
	cmp byte [CursorRowPtr], 23	;24 rows on the screen
	jge pm_linefeed_scroll
	inc byte [CursorRowPtr]
	jmp pm_next_char

	pm_linefeed_scroll:
	call PMScrollConsole

	pm_string_done:
	popf
	pop edx
	pop ecx
	pop eax
	pop edi
	pop es
	ret

;Print a substring of defined length in protected mode.
;Expects the string to print in ds:esi and the length in ecx. No null check is performed.
PMPrintStringLen:
	push es
	push edi
	push eax
	push edx
	pushf
	push ecx	;save the length from cx as we use it in the display offset calculation

	;set up es:edi to point to where we want to write the characters
	mov ax, DisplayMemorySeg
	mov es, ax
	mov edi, TextConsoleOffset

	;calculate the correct display offset based on the cursor position
	xor eax, eax
	mov al, byte [CursorRowPtr]	;assume 80 columns per row = 0x50. Double this to 0xa0 because each char is 2 bytes (char, attribute)
	mov cx, 0x00a0
	mul cx
	xor dx, dx
	mov dl, byte [CursorColPtr]
	add ax, dx			;AX is the offset
	add ax, dx			;add again to double the offset
	add edi, eax

	pop ecx		;restore the length into cx...

	pm_next_char_len:
	lodsb
	dec ecx
	test ecx,ecx
	jz pm_string_done_len	;if we get to zero, then exit
	cmp al, 0x0d		;CR
	jz pm_carraige_rtn_len
	cmp al, 0x0a		;LF
	jz pm_linefeed_len

	stosb
	mov al, DefaultTextAttribute
	stosb	;only use default attribute at the moment

	inc byte [CursorColPtr]
	cmp byte [CursorColPtr], 0x50	;columns-per-row
	jnz pm_next_char_len
	mov byte [CursorColPtr], 0
	inc byte [CursorRowPtr]
	jmp pm_next_char_len

	pm_carraige_rtn_len:
	mov byte [CursorColPtr], 0
	jmp pm_next_char_len

	pm_linefeed_len:
	cmp byte [CursorRowPtr], 23	;24 rows on the screen
	jge pm_linefeed_scroll_len
	inc byte [CursorRowPtr]
	jmp pm_next_char_len

	pm_linefeed_scroll_len:
	call PMScrollConsole

	pm_string_done_len:
	popf
	pop edx
	pop eax
	pop edi
	pop es
	ret
;Print a character in protected mode.
;Expects the character to print in bl.
PMPrintChar:
	push es
	push eax
	push dx
	mov ax, DisplayMemorySeg
	mov es, ax
	mov edi, TextConsoleOffset

	xor eax,eax
	mov al, byte [CursorRowPtr]
	mov cx, 0x00a0
	mul cx
	xor dx, dx
	mov dl, byte [CursorColPtr]
	add ax, dx
	add ax, dx
	add edi, eax

	mov al, bl
	stosb
	mov al, DefaultTextAttribute
	stosb

	inc byte [CursorColPtr]
	pop dx
	pop eax
	pop es
	ret

;Scrolls the console down by 1 line.
;Destroys AX, CX
PMScrollConsole:
	push es
	push esi
	push ds
	push edi

	mov ax, DisplayMemorySeg
	mov es, ax
	mov ds, ax

	;we want to copy all of the data _up_ one line, and fill the last with 0x20/0x00 (blank space) characters
	;es:edi is destination and ds:esi is source
	mov edi, TextConsoleOffset
	mov esi, TextConsoleOffset
	add esi, 0xA0		;80 characters * 2 bytes = 1 line
	mov ecx, 0x3C0		;80 characters * 24 lines * 2 bytes per char / 4 bytes per dword (into doublewords)
	rep movsd

	mov eax, 0x20
	mov ecx, 0x50		;1 line
	rep stosw

	pop edi
	pop ds
	pop esi
	pop es
	ret
