[BITS 16]
[ORG 0x7C00]	;boot loader lives here

;FAT format sector: https://networkencyclopedia.com/file-allocation-table-fat/#structure-of-fat-Volume
jmp _bootsect
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
mov ax, 0x1000 ;4k stack space after the boot-loader
mov ss, ax
mov sp, 0x2000

; set up data segment registers
mov ax, 0x07E0	;start of 480k conventional memory
mov es, ax
mov ds, ax

; print "hello" message
mov si, HelloString
call PrintString

; locate and load in ANDY.SYS. It's assumed that this is the first entry in the FAT.

mov si, LoadingStart
call PrintString

;check if int13 extensions are available
mov ax, 0x4100
mov bx, 0x55AA
mov dx, 0x0080
int 0x13
jc no_extns

call FindFirstFileEntry

loading_done:
mov si, LoadingDone
call PrintString
; in theory, if we get to here ANDY.SYS is loaded at 0x7E0:0000.... here goes nothing!
jmp 0x7E0:0000

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

LoadDiskSectors:
	;Loads disk sectors into memory, at 0x07E0(ds):0000. Overwrites this memory and clobbers registers.
	;Arguments: bx = number of sectors to read in, cx = (16-bit) sector offset. Assume 1 sector = 512 bytes.
	;set up a disk address packet structure at 0x07E0:800. stos* stores at es:di and increments. int13 needs struct in ds:si
	cld		;make sure we are writing forwards
	mov di, 0x800
	mov al, 0x0F	;16 byte packet
	stosb
	xor al,al	;field always 0
	stosb
	mov ax, bx	;transfer 16 sectors
	stosw
	xor ax,ax	;write to 0x07E0:0000. Offset first.
	stosw
	mov ax, ds	;segment to write
	stosw
	mov ax, cx	;we want to read one sector from offset of sector 4, but this is little-endian. Write 48 bits in 4 words - LSW first
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
	ret

FindFirstFileEntry:
	;Expect directory entries to be from offset 0x8800 on the disk, which is sector 0x44
	mov bx, 2
	mov cx, 0x0044
	call LoadDiskSectors
	mov si, 0x0b
	;now the first sector of the directory table is in memory at 0x07E0:0000. BUT the first entry could be an LFN.
next_entry:
	xor ax,ax
	mov al, byte [si]
	cmp al, 0x0f
	add si, 0x20
	jz next_entry
	
	;si now points to the attribute byte of the first entry that is not LFN. Subtract to get to the start of the record and check it's not null.
	sub si, 0x0b
	mov al, byte [si]
	test al, al	;sets Z flag if ax is zero
	jz fat_load_err

	add si, 0x1a	;file size is 4 bytes at offset 1c of the record. Assume it's <64k for the time being.
	mov ax, word [si+2]	;file size location
	mov bx, 0x200
	mul bx			;0x200 = 512, size of a sector in bytes
	mov bx, ax		;MUL result always in AX, with high word in DX
	mov ax, word [si]	;file start cluster. Assume that cluster size is 1800
	mov bx, 0x0c
	mul bx		;0x12=16 sectors per cluster
	add ax, 0x44		;data area starts at 0x8800
	mov cx, ax
	call LoadDiskSectors
	ret

;Data
HelloString db ':D', 0x0a, 0x0d, 0
CouldNotLoadErr db 'DiskError', 0x0a, 0x0d, 0
LoadingStart db 'Loading...', 0x0a, 0x0d, 0
LoadingDone db 'Done', 0x0a, 0x0d, 0
;ErrCodes db '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
TimeoutErr db 'Timeout!', 0x0a, 0x0d, 0
NoExtErr db 'BadBios', 0

;Fill the rest of the bootsector with nulls
TIMES 510 - ($ - $$) db 0
DW 0xAA55			; boot signature at the end of the bootsector
