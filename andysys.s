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

;Now set up Global Descriptor Table
mov ax,cs
mov ds,ax
mov es,ax
xor ax, ax
mov ax, SimpleGDT		;get the location of the GDT
add ax, 0x7E00			;add our base address at the start of conventional RAM
mov [SimpleGDTPtr+2], ax	;set the GDT location into the pointer structure
lgdt [SimpleGDTPtr]

mov si, GdtDone
call BiosPrintString

;Get the current cursor position
mov ax, 0x0300
mov bx, 0
int 0x10
;DH is row and DL is column

;now we are ready!
mov eax, cr0
or al, 1
mov cr0, eax	;hello protected mode
jmp 0x08:PModeMain+0x7E00

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
GdtDone db 'GDT ready.', 0x0a, 0x0d, 0

;basic GDT configuration. Each entry is 8 bytes long
SimpleGDT:
;entry 0: null entry
dd 0
dd 0
;entry 1 (segment 0x08): kernel CS
dw 0xffff	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x0000	;base bits 16-23
db 0x9A		;access byte. Set Pr, Privl=0, S=1, Ex=1, DC=0, RW=1, Ac=0
db 0x4f		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 2 (segment 0x10): kernel DS
dw 0xffff	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x10		;base bits 16-23. Start from 1meg.
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0x4f		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;FIXME: should set up TSS here
SimpleGDTPtr:
dw 0x18		;length in bytes
dd 0x0		;offset, filled in by code

%define CursorRowPtr 0x00	;where we store cursor row in kernel data segment
%define CursorColPtr 0x01	;where we store cursor col in kernel data segment

[BITS 32]
;we jump here once the switch to protected mode is complete, and we can use 32-bit code.
PModeMain:
;assuming that DH is still the cursor row and DL is still the cursor position

mov ax, 0x10
mov ds, ax	;set up segments to point to 32-bit data seg
mov es, ax
mov ss, ax
mov esp, 0x00fffff	;stack at the end of RAM data segment

mov byte [CursorRowPtr], dl
mov byte [CursorColPtr], dh

mov eax,0x01
push eax
mov eax, 0xFFFFFFFF
push eax

jmp $


