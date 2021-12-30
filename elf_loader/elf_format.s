; assuming 16-bits

%define ERR_OK 		0
%define ERR_NOT_ELF 	0x01
%define ERR_NOT_32bits	0x02
%define ERR_ENDIAN	0x03
%define ERR_INSTRUCTIONSET	0x04

;expects a pointer to elf file header in ds:si
;registers are NOT preserved
;returns an exit code in dx:
; 0x00 - success
; 0x01 - not an elf header
LoadElfBase:
	cmp word [ds:si], 0x457F	;header bytes reversed as we are little-endian
	jnz not_elf
	cmp word [ds:si+2], 0x464C
	jnz not_elf

	;we got elf. Due to space constraints we are just going to have to assume that it's 32-bit LE

	;ok. With that out of the way we need to get hold of the program header, set di to its location
	mov di, word [ds:si+0x1C]	;start of the program header table. https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
	add di, si			;add it to the base offset

	push si				;store our offsets (to the image base) on the stack in preparation for the move

	mov cx, word [ds:di+0x10]	;size of the segment in memory
	shr cx, 1			;quick way of dividing by 2

	mov dx, word [ds:di+0x04]	;COPY SOURCE - offset of the section in the file (ds:si => unchanged:base+offset)
	add dx, si
	mov si, dx

	mov ax, 0x7E0			;COPY DEST - move to the start of conventional memory (es:di 0x07E0:0)
	mov es, ax
	mov di,	0
	rep movsw			;copy the block of memory over. break 0x7d91

	pop si


	;now the code is loaded, load in the data from 0xc000
	;we have to assume that the .data section is immediately after the .text section
	mov di, word [ds:si+0x20]	;start of the section header table. break 00007d94
	add di, si			;add it to the base offset
	add di, 0x50			;we are making an assumption here that the structure of the file has the .data section here
	mov cx, word [ds:di+0x14]	;size, in bytes of the section
	shr cx, 1

	mov dx, word [ds:di+0x10]	;COPY SOURCE - offset of the data in the file
	add dx, si
	mov si, dx

	mov ax, 0xC00			;COPY DEST move to conventional memory at 0xc000
	mov es, ax
	mov di, 0
	rep movsw			;break 00007db3

	mov dx,0
	ret
not_elf:
	mov dx, ERR_NOT_ELF
	ret
