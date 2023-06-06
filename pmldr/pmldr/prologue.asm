;The purpose of the "prologue" is to take us from 16-bit to 32-bit mode so we can prepare to load the main kernel.
[BITS 16]
[ORG 0x7E00]

;We are going to put the disk parameters buffer at 0x50:0x00 (0x500 in PM)

%include "../../andy_elf/memlayout.asm" ;ensure that e.g. TemporaryMemInfoBufferSeg is the same as for the kernel

;before we go to PM, retrieve the bios memory map
call detect_memory

;also, get the boot drive's parameters. DL is already set to the BIOS disk number.
call get_boot_drive_params

;Try to enter protected mode - https://wiki.osdev.org/Protected_Mode
;Disable interrupts
cli

;Disable NMI - https://wiki.osdev.org/Non_Maskable_Interrupt
in AL, 0x70
or al, 0x80
out 0x70, al
in al, 0x71

in al, 0x92	;if a20 not enabled, do it now
or al, 2
out 0x92, al
a20_enabled:

;Now set up Global Descriptor Table
mov ax, cs
mov ds, ax
xor eax, eax
mov eax, SimpleGDT		;get the location of the GDT, add the absolute location of our data segment
mov si, SimpleGDTPtr
sub si, 0x7E00
mov [si+2], eax	;set the GDT location into the pointer structure
lgdt [si]

;Get the current cursor position
mov ax, 0x0300
mov bx, 0
int 0x10
;DH is row and DL is column

;now we are ready!
mov eax, cr0
or al, 1
mov cr0, eax	;hello protected mode
jmp 0x08:_pm_start	;Here goes nothing!


;retrieve the BIOS memory map. This must be compiled and run in 16-bit mode, easiest way
;is to call it before the switch to PM.
detect_memory:
	mov ax, TemporaryMemInfoBufferSeg  ;target location is 0x250:000
	mov es, ax
	mov di, 2
	xor si, si	;use si as a counter
	xor ebx,ebx
	mem_det_loop:
	mov edx, 0x534d4150
	mov eax, 0xe820
    mov ecx, 24

	inc si
	int 0x15
	jc mem_det_done
	test ebx, ebx
	jz mem_det_done

	add di, 24
	jmp mem_det_loop

	mem_det_done:
	mov word [es:000], si
	ret

get_boot_drive_params:
	push ebp
	mov ebp, esp

	;disk info buffer goes at 0x500 in conventional RAM, i.e. 0x50:0x00 in seg/offset
	mov ax, 0x50
	mov ds, ax
	mov si, 0x0
	mov ah, 0x48
	int 0x13
	jc drive_param_err
	pop ebp
	ret

drive_param_err:
	push cs
	pop ds
	mov si, DriveParamErrString
	call PrintString
	jmp $
	
;Data
HelloString db 'PMLDR v1.0', 0
DriveParamErrString db 'Unable to determine boot drive params\n', 0
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
db 0xCF		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 2 (segment 0x10): kernel DS. Allow this to span the whole addressable space.
dw 0xffff	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x00		;base bits 16-23. Start from 0meg.
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0xCf		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 3 (segment 0x18): video RAM. See https://wiki.osdev.org/Memory_Map_(x86).
dw 0xFFFF	; limit bits 0-15.
dw 0x0000	; base bits 0-15
db 0x0A		; base bits 16-23. Start from 0x0A0000
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0x41		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31


SimpleGDTPtr:
dw 0x20		;length in bytes
dd 0x0		;offset, filled in by code