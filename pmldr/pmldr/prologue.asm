;The purpose of the "prologue" is to take us from 16-bit to 32-bit mode so we can prepare to load the main kernel.
[BITS 16]
[ORG 0x7E00]

%include "../../andy_elf/memlayout.asm" ;ensure that e.g. TemporaryMemInfoBufferSeg is the same as for the kernel

xor ebp, ebp
mov ax, 0
mov ds, ax
mov byte [ds:BiosBootDevicePtr], dl 	;dl should be the disk number

;hello world
mov si, HelloString
sub si, 0x7e00
call PrintString

call get_boot_drive_params		;FIXME - the BIOS now reports the number of bytes per sector, we should use that insted of the hard-coded value

mov si, LocKernString
sub si, 0x7e00
call PrintString

;Load the kernel
xor ecx, ecx				;ensure that the upper word of ecx is clear, we rely on this when loading low sector numbers.
call LoadBootSector
mov ax, BootSectorMemSectr
mov ds, ax
call LoadFAT
call LoadRootDirectory
call FindKernelBinary
call LoadInKernel

;before we go to PM, retrieve the bios memory map
mov si, MemDetString
sub si, 0x7e00
call PrintString

call detect_memory

call detect_pci

mov si, PrepString
sub si, 0x7e00
call PrintString
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

mov si, DoneString
sub si, 0x7e00
call PrintString

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
	push ebp
	mov ebp, esp
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
	pop ebp
	ret

;retrieve the PCI 32-bit entrypoint
detect_pci:
	push ebp
	mov ebp, esp
	xor edi, edi
	mov ax, 0xB101
	int 0x1A

	jc .no_pci	;docs say that CF should be clear
	test ah, ah	;docs say that AH should be 0 if PCI BIOS present
	jnz .no_pci

	push edi
	mov ax, TemporaryPciInfoBufferSeg
	mov es, ax
	mov di, TemporaryPciInfoBufferOffset
	pop eax	;take the EDI value from the call into EAX
	stosd

	mov si, PciDetectedString
	sub si, 0x7e00
	call PrintString

	.det_pci_done:
	pop ebp
	ret

	.no_pci:
	mov si, NoPciString
	sub si, 0x7e00
	call PrintString
	jmp .det_pci_done

; uses BIOS int10 to output the character in AL to the screen
PrintCharacter:
	mov ah, 0x0E
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	ret

; repeatedly calls PrintCharacter to output the ASCIIZ string pointed to by the SI register
PrintString:
	push ebp
	mov ebp, esp
	next_char:
	mov al, [cs:si]
	inc si
	or al, al	;sets zero flag if al is 0 => end of string
	jz exit_function
	call PrintCharacter
	jmp next_char
	exit_function:
	pop ebp
	ret

get_boot_drive_params:
	push ebp
	mov ebp, esp

	xor ax, ax
	mov es, ax
	;disk info buffer goes at 0x3100 in conventional RAM, i.e. 0x310:0x00 in seg/offset
	mov ax, 0x310
	mov ds, ax
	mov si, 0x0
	mov word [ds:si], 0x42					;request version 3 metadata
	mov ax, 0x4800
	mov dl, byte [es:BiosBootDevicePtr]
	int 0x13
	jc drive_param_err
	pop ebp
	ret

drive_param_err:
	push cs
	pop ds
	mov si, DriveParamErrString
	sub si, 0x7e00
	call PrintString
	jmp $
	
;compares the strings in ds:si and es:di up to a maximum number of characters stored in cx.
;returns ax=0 on match, otherwise nonzero
strncmp:
	push ebp
	mov ebp, esp

	push bx

	_next_char:
	test cx, cx
	jz _strncmp_success	;cx=0 => we got to end of the string

	dec cx
	mov bh, byte [ds:si]
	mov bl, byte [es:di]
	inc si
	inc di
	cmp bh, bl
	jz _next_char

	;if we got here then one character did not match
	mov ax, 1
	pop bx
	pop ebp
	ret

	_strncmp_success:	;if we got here then we hit the end without any mismatch
	mov ax,0
	pop bx
	pop ebp
	ret

;Data
HelloString db 0x0d, 0x0a, 'PMLDR v1.0', 0x0d, 0x0a, 0
MemDetString db 'Detecting RAM...', 0x0d, 0x0a, 0
LocKernString db 'Locating kernel...', 0x0d, 0x0a, 0
BootParamsString db 'Detecting boot drive...', 0x0d, 0x0a, 0
PrepString db 'Entering protected mode...', 0x0d, 0x0a, 0
DoneString db 'Moving on', 0x0d, 0x0a, 0
DriveParamErrString db 'Unable to determine boot drive params', 0x0d, 0x0a, 0
NoPciString db 'No PCI present.', 0x0d, 0x0a, 0
PciDetectedString db 'PCI BIOS detected.', 0x0d, 0x0a, 0

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