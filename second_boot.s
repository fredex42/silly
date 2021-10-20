[BITS 16]
[ORG 0x0000]	;set up sectors manually

;FAT format sector: https://networkencyclopedia.com/file-allocation-table-fat/#structure-of-fat-Volume
jmp 0x7c0:_bootsect
db 'ANDY_____'
dw	0x0200	;512 bytes per logical sector
db	0x04	;4 logical sectors per cluster - =>16-bit FAT
dw	0x0004	;4 reserved sectors, including this one! FAT 0 is at 2k => 0x800 bytes
db	0x02	;2 FATs
dw	0x200	;max. 512 root dir entries
dw	0x0020	;0x20 logical sectors => 16Mbyte disk
db	0xF8	;media descriptor for hard disk
dw	0x0020	;one FAT, all logical sectors are in that
TIMES 24 db 0	;no extended parameter block

_bootsect:
; set up code segment for bootloader
; set up stack
mov ax, 0x07C0 ;4k stack space after the boot-loader
mov ss, ax
mov sp, 4096

; set up data segment registers
mov ax, 0x07E0	;start of 480k conventional memory
mov es, ax
mov ds, ax

; print "hello" message
mov si, HelloString
call PrintString

; locate and load in ANDY.SYS. It's assumed that this is the first entry in the FAT.

;load first sector of FAT 0 into memory
;mov ax, 0x07E0	;start of 480k conventional memory
;mov es, ax
;mov ds, ax	;used for reading in the data, below
;mov bx,	0	;load in at the start of RAM
;mov ax, 0x0201	;read in 1 sector
;mov cx, 0x0004	;start at cylinder 0, sector 4 => 2k into disk
;mov dx, 0x0080	; read from head 0 of HD 0. Drive number 0x80 => HD 0 (otherwise are floppies)
;int 0x13
;jc fat_load_err

mov si, LoadingStart
call PrintString

;check if int13 extensions are available
mov ax, 0x4100
mov bx, 0x55AA
mov dx, 0x0080
int 0x13
jc no_extns

;load the first FAT into memory
;set up a disk address packet structure at 0x07E0:800. stos* stores at es:di and increments. int13 needs struct in ds:si
cld		;make sure we are writing forwards
mov di, 0x800
mov al, 0x0F	;16 byte packet
stosb
xor al,al	;field always 0
stosb
mov ax, 0x01	;transfer 16 sectors
stosw
xor ax,ax	;write to 0x07E0:0000. Offset first.
stosw
mov ax, ds	;segment to write
stosw
mov ax, 0x0300	;we want to read one sector from offset of sector 4, but this is little-endian. Write 64 bits in 4 words.
stosw
xor ax,ax
stosw
stosw
stosw		;now we are ready
mov si, 0x800
mov ah, 0x42
mov dl, 0x80
int 0x13
jc fat_load_err

;now get the first file from the first FAT
mov si, 0x0004	;FAT is read in from 0x07E0:0000. We want to read from offset 4 which is the first file cluster.
		; these go in ds:si
next_cluster:
lodsw		;ax is now the cluster number

loading_done:
mov si, LoadingDone
call PrintString
jmp _hang

fat_load_err:
cmp ah, 0x80	;0x80 is a timeout error and gets its own message
jz fat_load_timeout

mov si, CouldNotLoadErr
call PrintString
jmp _hang

fat_load_timeout:
mov si, TimeoutErr
call PrintString
jmp _hang

no_extns:
mov si, NoExtErr
call PrintString
jmp _hang

_hang:
jmp $

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
	mov al, [cs:si]
	inc si
	or al, al	;sets zero flag if al is 0 => end of string
	jz exit_function
	call PrintCharacter
	jmp next_char
	exit_function:
	ret

;Data
HelloString db ':D', 0x0a, 0x0d, 0
CouldNotLoadErr db 'NoFAT', 0x0a, 0x0d, 0
LoadingStart db 'Loading...', 0x0a, 0x0d, 0
LoadingDone db 'Done', 0x0a, 0x0d, 0
;ErrCodes db '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
TimeoutErr db 'Timeout!', 0x0a, 0x0d, 0
NoExtErr db 'BadBios', 0

;Fill the rest of the bootsector with nulls
TIMES 510 - ($ - $$) db 0
DW 0xAA55			; boot signature at the end of the bootsector
