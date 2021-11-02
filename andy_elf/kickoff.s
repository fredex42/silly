[BITS 32]
section .text
global _start

%include "memlayout.inc"
;assuming that DH is still the cursor row and DL is still the cursor position
_start:

;Now we need to re-set up the selectors as what we got from the bootloader was fairly minimal.
;The new one here has the code and data segments the same but adds in framebuffer ram and TSS
mov ax, 0x10 
mov ds, ax
mov es, ax
mov ss, ax
mov esp, 0x7fff0	;set stack pointer to the end of conventional RAM
xor ax, ax
mov ax, SimpleGDT
mov [SimpleGDTPtr+2], ax
lgdt [SimpleGDTPtr]

mov byte [CursorColPtr], dl
mov byte [CursorRowPtr], dh

mov esi, HelloString
call PMPrintString

jmp $

;Print a string in protected mode.
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

	reset_offset:
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
	
	stosb
	mov al, DefaultTextAttribute
	stosb	;only use default attribute at the moment

	jmp pm_next_char

	pm_string_done:
	popf
	pop edx
	pop ecx
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

section .data
HelloString: db 'Hello world', 0

;basic GDT configuration. Each entry is 8 bytes long
SimpleGDT:
;entry 0: null entry
dd 0
dd 0
;entry 1 (segment 0x08): kernel CS
dw 0xffff	;limit bits 0-15
dw 0x7E00	;base bits 0-15
db 0x0000	;base bits 16-23
db 0x9A		;access byte. Set Pr, Privl=0, S=1, Ex=1, DC=0, RW=1, Ac=0
db 0x47		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 2 (segment 0x10): kernel DS
dw 0xffff	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x10		;base bits 16-23. Start from 1meg.
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0x47		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 3 (segment 0x18): video RAM. See https://wiki.osdev.org/Memory_Map_(x86).
dw 0xFFFF	; limit bits 0-15.
dw 0x0000	; base bits 0-15
db 0x0A		; base bits 16-23. Start from 0x0A0000
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0x41		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 4 (segment 0x20): TSS. See https://wiki.osdev.org/Task_State_Segment.
dw 0x68		;limit bits 0-15
dw 0xff00	;base bits 0-15.
db 0x10		;base bits 16-23. Start from end of kernel DS
db 0x84		;access byte. Set Pr, Privl=0, S=0, Ex=0, DC=0, RW=1,Ac=0
db 0x40		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31

SimpleGDTPtr:
dw 0x28		;length in bytes
dd 0x0		;offset, filled in by code


