[BITS 16]

LoadDiskSectors:
	push ebp
	mov ebp, esp
;Loads disk sectors into memory, at (ds):0000. Overwrites this memory and clobbers registers.
;Arguments: bx = number of sectors to read in, cx = (16-bit) sector offset. Assume 1 sector = 512 bytes.
;set up a disk address packet structure at 0x7000:000. stos* stores at es:di and increments. int13 needs struct in ds:si
	;this is a new version that loads one sector at a time, to accomodate bios quirks (see https://stackoverflow.com/questions/58564895/problem-with-bios-int-13h-read-sectors-from-drive)
	cld		;make sure we are writing forwards
	push es
	push ds
	mov ax, 0x7000
	mov es, ax

	mov di, 0x00
	mov al, 0x10	;16 byte packet
	stosb
	xor al,al	;field always 0
	stosb
	mov ax, 1
	stosw
	xor ax,ax	;write to sector origin
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
	mov si, 0x00
	mov ah, 0x42
	mov dl, 0x80	;FIXME: hardcoded disk number!
	int 0x13
	jc fat_load_err	;this hangs the bootloader so it does not really matter that we leave two values on the stack
	dec bx					;track how many sectors we want to load in bx
	test bx,bx
	jz _load_disk_sectors_done
	add word [es:0x06], 0x020	;each block is 512 bytes = 0x200. Increment the destination ptr by this much.
	inc dword [es:0x08]				;increment the block number we want to read
	mov word [es:0x02], 1			;blocks-to-transfer is overwritten by blocks transferred after the call, so reset it
	jmp _load_next_sector

	_load_disk_sectors_done:
	pop ds
	pop es
	pop ebp
	ret

LoadBootSector:
	push ebp
	mov ebp, esp
	;disk buffer location 0x2500 (0x250:0)
	mov ax, 0x250
	mov ds, ax
	;boot sector image 0x500 (0x0:0x500)
	xor ax, ax
	mov es, ax

	mov bx, 1
	mov cx, 0
	call LoadDiskSectors

	mov si, 0
	mov di, 0x500
	mov cx, 0x100
	rep movsw

	;Now, we have a little problem. A "sector" in the disk format is not necessarily a "sector" in the hardware.
	;So, we calculate a "fudge factor" here.
	mov ax, BootSectorMemSectr
	mov ds, ax
	xor dx, dx
	mov ax, word [BytesPerSector]
	mov bx, 0x200
	div bx
	mov bh, 0
	mov bl, byte [SectorsPerCluster]
	mul bx
	mov word [CustomDiskBlocksPerCluster], ax

	pop ebp
	ret

;Expects ds to point to the start of the boot sector image, i.e. 0x50
;result is returned in bx
GetSectorsPerFAT:
	push ebp
	mov ebp, esp
	mov bx, word [FAT16SectorsPerFAT]
	test bx, bx
	jnz _get_sec_per_fat_done
	mov bx, word [FAT32SectorsPerFAT]
_get_sec_per_fat_done:
	pop ebp
	ret

;Loads the first cluster (4 sectors) of the File Allocation Table into memory
;Expects ds to point to the start of the boot sector image, i.e. 0x50
LoadFAT:
	push ebp
	mov ebp, esp
	
	mov ax, [ReservedSectors]	;FAT area starts immediately after the reserved sectors
	mov bx, [CustomDiskBlocksPerCluster]
	mul bx
	mov cx, ax
	mov bx, 4

	push ds
	mov ax, 0x70
	mov ds, ax
	call LoadDiskSectors

	pop ds
	pop ebp
	ret

;Expects ds to point to the start of the boot sector image, i.e. 0x50
;Returns the cluster offset of the root directory for FAT32 in dx:ax
F32GetRootDirLocation:
	push ebp
	mov ebp, esp
	xor ax, ax
	mov ax, word [CustomDiskBlocksPerCluster]
	mov dx, word [FAT32RootDirStart]
	add dx, word [ReservedSectors]
	mul dx
	
	pop ebp
	ret

;Loads in the root directory to memory buffers.
;Expects to find the boot sector at 0x500
LoadRootDirectory:
	push ebp
	mov ebp, esp

	mov ax, BootSectorMemSectr
	mov ds, ax

	;set cx to on-disk location of the root directory
	call F32GetRootDirLocation
	mov cx, ax
	;load in 1 cluster
	xor ax, ax
	mov al, byte [SectorsPerCluster]
	mov dx, word [CustomDiskBlocksPerCluster]
	mul dx
	mov bx, ax

	;load the data at 0xF00
	mov ax, 0xF0
	mov ds, ax

	call LoadDiskSectors
	pop ebp
	ret

FindFirstFileEntry:
	;Expect directory entries to be from offset 0x8800 on the disk, which is sector 0x44
	mov bx, 2
	mov cx, 0x0044
	call LoadDiskSectors
	mov si, 0x0b
	;now the first sector of the directory table is in memory at ds:0000. BUT the first entry could be an LFN.
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

	add si, 0x1a	;file size is 4 bytes at offset 1c of the record. Assume it's <64k for the time being. break 00007d48
	mov ax, word [si+2]	;file size location
	mov bx, 0x200
	xor dx,dx	;make sure dx is 0 as divide source is dx:ax
	div bx			;0x200 = 512, size of a sector in bytes
	push ax		;DIV result always in AX, with high word in DX. Save it for later
	mov ax, word [si]	;file start cluster. Assume that cluster size is 1800
	mov bx, 0x0c
	mul bx		;0x12=16 sectors per cluster
	add ax, 0x44		;data area starts at 0x8800
	mov cx, ax
	pop bx		;set bx to the number of sectors to load. We saved this earlier from ax.
	add bx,1	;bx is the lower bound of integer division, so there is probably a partial sector afterwards. Load this in too.
	call LoadDiskSectors
	ret

fat_load_err:
	mov si, FSErrString
	sub si, 0x7E00
	call PrintString
	jmp $

;data
FSErrString db 'Could not load filesystem', 0x0d, 0x0a, 0


