[BITS 16]
;Source code of andy.sys. This file is called by the bootloader.

mov si, HelloString
call BiosPrintString

;Try to enter protected mode - https://wiki.osdev.org/Protected_Mode
;Disable interrupts
cli
;Disable NMI - https://wiki.osdev.org/Non_Maskable_Interrupt
in AL, 0x70
or al, 0x80
out 0x70, al
in al, 0x71
mov si, TestA20
call BiosPrintString
;Enable the a20 line if needed
call check_a20
test ax,1
jz a20_enabled

in al, 0x92	;a20 not enabled, do it now
or al, 2
out 0x92, al
a20_enabled:
mov si, A20En
call BiosPrintString

jmp $

; Function: check_a20
;
; Purpose: to check the status of the a20 line in a completely self-contained state-preserving way.
;          The function can be modified as necessary by removing push's at the beginning and their
;          respective pop's at the end if complete self-containment is not required.
;
; Returns: 0 in ax if the a20 line is disabled (memory wraps around)
;          1 in ax if the a20 line is enabled (memory does not wrap around)
 
check_a20:
    xor ax, ax ; ax = 0
    mov es, ax
 
    not ax ; ax = 0xFFFF
    mov ds, ax
 
    mov di, 0x0500
    mov si, 0x0510
 
    mov al, byte [es:di]
    push ax
 
    mov al, byte [ds:si]
    push ax
 
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
 
    cmp byte [es:di], 0xFF
 
    pop ax
    mov byte [ds:si], al
 
    pop ax
    mov byte [es:di], al
 
    mov ax, 0
    je check_a20__exit
    mov ax, 1
 
check_a20__exit:
    ret

; uses BIOS int10 to output the character in AL to the screen
BiosPrintCharacter:
	mov ah, 0x0E
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	ret

; repeatedly calls PrintCharacter to output the ASCIIZ string pointed to by the SI register
BiosPrintString:
	next_char:
	mov al, [cs:si]
	inc si
	or al, al	;sets zero flag if al is 0 => end of string
	jz exit_function
	call BiosPrintCharacter
	jmp next_char
	exit_function:
	ret

;Data
HelloString db 'ANDY.SYS v1', 0x0a,0x0d,0
TestA20 db 'TestingA20', 0x0a,0x0d,0
A20En db 'A20 enabled.', 0x0a,0x0d,0

