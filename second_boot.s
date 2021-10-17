[BITS 16]
[ORG 0x7c00]	;bootsector goes here in memory

;FAT format sector: https://networkencyclopedia.com/file-allocation-table-fat/#structure-of-fat-Volume
jmp _bootsect
db 'ANDY_____'
dw	0x0200	;512 bytes per logical sector
db	0x04	;4 logical sectors per cluster - =>16-bit FAT
dw	0x0004	;4 reserved sectors, including this one! FAT 0 is at 2k => 0x800 bytes
db	0x02	;2 FATs
dw	0x200	;max. 512 root dir entries
dw	0x0020	;0x20 logical sectors => 16Mbyte disk
db	0xF8	;media descriptor for haard disk
dw	0x0020	;one FAT, all logical sectors are in that
TIMES 24 db 0	;no extended parameter block

_bootsect:
; set up stack
mov ax, 0x07C0	;4k stack space after the boot-loader
mov ss, ax
mov sp, 4096

; print "hello" message
mov si, HelloString
call PrintString

; locate and load in ANDY.SYS. It's assumed that this is the first entry in the FAT.

;load first sector of FAT 0 into memory
mov ax, 0x07E0	;start of 480k conventional memory
mov es, ax
mov ds, ax	;used for reading in the data, below
mov bx,	0	;load in at the start of RAM
mov ax, 0x0201	;read in 1 sector
mov cx, 0x0004	;start at cylinder 0, sector 4 => 2k into disk
mov dx, 0x0080	; read from head 0 of HD 0. Drive number 0x80 => HD 0 (otherwise are floppies)
int 0x13
jc fat_load_err

mov si, LoadingStart
call PrintString

mov si, 2		;set up. Read from offset 4 in the table until the next 0xFF
mov bx, 0 
cld				;make sure we are reading forwards

load_next_cluster:		;set up a loop to load the clusters from the first chain in the table.
	lodsw			;load data at ds:si into ax. This is the number of the next cluster.
	cmp ax, 0xFFFF
jz loading_done
	mov cx, ax
	call LoadCluster
jmp load_next_cluster

loading_done:
mov si, LoadingDone
call PrintString
jmp _hang


fat_load_err:
cmp ah, 0x80
jz fat_load_timeout
;mov al, ah	;status code is in ah, move to al
;xor ah, ah	;zero ah
;mov si, ax
;mov al, byte [ErrCodes+si]
;call PrintCharacter

mov si, CouldNotLoadErr
call PrintString
jmp _hang

fat_load_timeout:
mov si, TimeoutErr
call PrintString
jmp _hang

_hang:
jmp $

; uses BIOS int 0x13 to load in the cluster (4 sectors) with number in cl to the location pointed by es:bx
LoadCluster:
	push ax
	push dx
	
	;multiply the cluster number by 4 to get the sector number. This goes in CX
	mov ax,cx
	and ax, 0x00FF	;make sure ah is 0
	mov dx, 0x04
	mul dx
	mov cx,ax
	
	mov ax,0x0204	;ah is the instruction code, al is the sector count to read
	mov dx, 0x0080	;dh is the head number, dl is the drive numnber. DX bit 7 => hard disk
	int 0x13	;read data into es:bx

	pop dx
	pop ax
	ret

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
HelloString db 'Hello World', 0x0a, 0x0d, 0
CouldNotLoadErr db 'No FAT', 0x0a, 0x0d, 0
LoadingStart db 'Loading file..', 0x0a, 0x0d, 0
LoadingDone db 'Loading complete.', 0x0a, 0x0d, 0
;ErrCodes db '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
TimeoutErr db 'Timeout!', 0x0a, 0x0d, 0

;Fill the rest of the bootsector with nulls
TIMES 510 - ($ - $$) db 0
DW 0xAA55			; boot signature at the end of the bootsector
