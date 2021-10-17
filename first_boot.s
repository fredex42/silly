; This file represents a very basic bootsector
[BITS 16]
[ORG 0x7c00]

mov si, HelloString
call PrintString
jmp $	; infinite loop, hang the emulator

; uses BIOS int10 to output the character in AL to the screen
PrintCharacter:
	mov ah, 0x0E
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	ret

; repeatedly calls PrintCharacter to output the ASCIIZ string pointed to by the SI register
PrintString:
	next_char:
	mov al, [si]
	inc si
	or al, al	;sets zero flag if al is 0 => end of string
	jz exit_function
	call PrintCharacter
	jmp next_char
	exit_function:
	ret

;Data
HelloString db 'Hello World', 0

;Fill the rest of the bootsector with nulls
TIMES 510 - ($ - $$) db 0
DW 0xAA55			; boot signature at the end of the bootsector
