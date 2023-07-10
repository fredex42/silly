[BITS 16]
[ORG 0x7C68]	;boot loader lives here, this is above the disk formatting information

; These values are written by install_loader during the installation process and point us to the location of the PMLDR binary on-disk.
; That way we don't have to waste valuable space interpreting the disk format
pmldr_sector: dw 0
pmldr_length: dw 0

_bootsect:
; set up code segment for bootloader
; set up stack
mov ax, 0x7000 ;4k stack space below the boot-loader, grows downwards to 0x50
mov ss, ax
mov sp, 0

; set up data segment registers
mov ax, 0x4000	;area below our stack and well above our code, which is at 0x7C00 (seg. 0x7C0) and the loading area at 0x7E00. Gives 192k of load space.
mov es, ax
mov ds, ax

mov si, LoadingStart
call PrintString

;Store the boot device
mov byte [BootDevice], dl

;check if int13 extensions are available
mov ax, 0x4100
mov bx, 0x55AA
mov dx, 0x0080
int 0x13
jc no_extns

xor ax, ax
mov ds, ax
mov cx, word [pmldr_sector]
mov bx, word [pmldr_length]
mov ax, 0x7E0       ;we want to load PMLDR at 0x7E0:0000
mov ds, ax          ;so set ds to 0x7E0
call LoadDiskSectors	;break 7CA3

;Right, now we are loaded.  Jump into PMLDR.
mov si, LoadingDone		;break 7CA9
call PrintString
mov dl, byte [BootDevice]
jmp 0x7E0:0x0000

;We can't continue if int13 extensions are not available.
no_extns:
    mov si, NoExtErr
    call PrintString
    jmp $

; uses BIOS int10 to output the character in AL to the screen
PrintCharacter:
	mov ah, 0x0E
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	ret

; repeatedly calls PrintCharacter to output the ASCIIZ string pointed to by the SI register relative to the code segment
PrintString:
	.next_char:
	mov al, [cs:si]
	inc si
	or al, al	;sets zero flag if al is 0 => end of string
	jz .exit_function
	call PrintCharacter
	jmp .next_char
	.exit_function:
	ret


LoadDiskSectors:
;Loads disk sectors into memory, at 0x07E0(ds):0000. Overwrites this memory and clobbers registers.
;Arguments: bx = number of sectors to read in, cx = (16-bit) sector offset. Assume 1 sector = 512 bytes.

	;this is a new version that loads one sector at a time, to accomodate bios quirks (see https://stackoverflow.com/questions/58564895/problem-with-bios-int-13h-read-sectors-from-drive)
	cld		;make sure we are writing forwards
	push es
	push ds
	mov ax, 0x250
	mov es, ax

    ;set up a disk address packet structure at 0x0250:000. stos* stores at es:di and increments. int13 needs struct in ds:si
	mov di, 0x0000
	mov al, 0x10	;16 byte packet
	stosb
	xor al,al	;field always 0
	stosb
	mov ax, 1   ;always transfer 1 block at a time (see notes above)
	stosw
	xor ax,ax	;write to segment origin
	stosw
	mov ax, ds	;segment to write
	stosw
	mov ax, cx	;we want to read one sector from offset of sector 4, but this is little-endian. Write 48 bits in 4 words - LSW first
	stosw
	xor ax,ax
	stosw
	stosw
	stosw		;now we are ready
	mov ax, es
	mov ds, ax
	xor ax, ax

	_load_next_sector:
	xor si, si
	mov ah, 0x42
	mov dl, 0x80
	int 0x13
	jc disk_load_err	;this hangs the bootloader so it does not really matter that we leave two values on the stack
	dec bx					;track how many sectors we want to load in bx
	test bx,bx
	jz _load_disk_sectors_done
	add word [es:0x806], 0x020	;each block is 512 bytes = 0x200. Increment the destination ptr by this much.
	inc dword [es:0x808]				;increment the block number we want to read
	mov word [es:0x802], 1			;blocks-to-transfer is overwritten by blocks transferred after the call, so reset it
	jmp _load_next_sector

	_load_disk_sectors_done:
	pop ds
	pop es
	ret

disk_load_err:
    mov si, CouldNotLoadErr
    call PrintString
    nop
    jmp $

;Data
BootDevice      db 0    ;this is filled in from the dl register at startup
CouldNotLoadErr db 'Could not load', 0
LoadingStart db 'Loading...', 0
LoadingDone db 'done.', 0
LoadErr db 'BadELF', 0
;ErrCodes db '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
TimeoutErr db 'T!', 0
NoExtErr db 'No int13 extensions', 0