[BITS 16]

LoadDiskSectors:
	push ebp
	mov ebp, esp
	xor ax, ax
	mov fs, ax

;Loads disk sectors into memory, at (ds):0000. Overwrites this memory and clobbers registers.
;Arguments: bx = number of sectors to read in, ecx = (32-bit) sector offset. Assume 1 sector = 512 bytes.
;set up a disk address packet structure at 0x300:000. stos* stores at es:di and increments. int13 needs struct in ds:si
	;this is a new version that loads one sector at a time, to accomodate bios quirks (see https://stackoverflow.com/questions/58564895/problem-with-bios-int-13h-read-sectors-from-drive)
	cld		;make sure we are writing forwards
	push es
	push ds
	mov ax, 0x300
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
	mov eax, ecx	;Write 48 bits in 4 words - LSW first
	stosd
	xor ax,ax
	stosw
	stosw		;now we are ready
	mov ax, es
	mov ds, ax
	xor eax, eax

	_load_next_sector:
	mov si, 0x00
	mov ah, 0x42
	mov dl, [fs:BiosBootDevicePtr]
	int 0x13
	jc fat_load_err					;this hangs the bootloader so it does not really matter that we leave two values on the stack
	dec bx							;track how many sectors we want to load in bx
	test bx,bx
	jz _load_disk_sectors_done
	add word [es:0x06], 0x020		;each block is 512 bytes = 0x200. Increment the destination ptr by this much. FIXME this block size should be derived from the GET DRIVE PARAMETERS (int0x15/0x48 command)
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
	mov bh, 0
	mov bl, byte [SectorsPerCluster]
	mul bx
	mov bx, 0x200					;FIXME - assume that disk always has 512 (=0x200) byte sectors
	div bx

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
	
	mov cx, [ReservedSectors]	;FAT area starts immediately after the reserved sectors. FIXME: assuming that disk sector = hw sector
	mov bx, 4

	push ds
	mov ax, 0x70
	mov ds, ax
	call LoadDiskSectors

	pop ds
	pop ebp
	ret

;Expects ds to point to the start of the boot sector image, i.e. 0x50
;Returns the sector offset of the root directory for FAT32 in dx:ax
F32GetRootDirLocation:
	push ebp
	mov ebp, esp
	xor ax, ax
	mov ax, word [FAT32SectorsPerFAT]
	mov dx, word [FAT32SectorsPerFAT+2]
	mov bh, 0
	mov bl, byte [FATCount]
	mul bx
	add ax, word [ReservedSectors]
	
	pop ebp
	ret

;Loads in the root directory to memory buffers.
;Expects to find the boot sector at 0x500
LoadRootDirectory:
	push ebp
	mov ebp, esp

	mov ax, BootSectorMemSectr
	mov ds, ax

	cmp word [FAT16RootDirEnt], 0
	jnz .LRDFat16

	;set cx to on-disk location of the root directory
	call F32GetRootDirLocation
	jmp .LRDCont

	.LRDFat16:
	;we don't currently support FAT16 load
	mov si, [WrongFATErr]
	sub si, 0x7e00
	call PrintString
	hlt
	jmp $-2

	.LRDCont:
	mov cx, ax
	;load in 1 cluster
	xor ax, ax
	mov al, byte [SectorsPerCluster]
	mov dx, word [CustomDiskBlocksPerCluster]
	mul dx
	mov bx, ax

	cmp bx, 0x0B				;if we load more than 0x0B hardware sectors then we clobber other memory (see memory map in pmldr.asm)
	jng _root_dir_load_cont
	mov bx, 0x0B

	_root_dir_load_cont:
	;load the data at 0xF00
	mov ax, 0xF0
	mov ds, ax

	call LoadDiskSectors
	pop ebp
	ret

;Tries to locate the kernel binary (ANDY.SYS) in the root directory
FindKernelBinary:
	push ebp
	mov ebp, esp

	mov ax, 0xF0	;start of root dir
	mov ds, ax
	xor ax, ax
	mov es, ax
	mov ax, 0x0
	mov fs, ax										;fs points to the origin, which our local offsets are measured to.

	;Each directory entry is 32 (0x20) bytes long.  Use bx to track which one we are on
	mov bx, 0

	_next_entry:
	cmp bx, 0xFF
	jz _kern_not_found		;break after we've looped 255 times. FIXME - should be a bit more scientific about this!
	mov ax, 0x20
	mul bx
	inc bx

	mov si, ax				;the file name is 11 chars (bytes) at the start of the entry, i.e. (entry_count * 0x20)
	mov di, KernelName		;compare this to the stored expected name
	mov cx, 11				;compare this many chars
	call strncmp			;should preserve bx. AX=0 on success
	test ax, ax
	jnz _next_entry			;we didn't find one

	;if we got here, we have a match! We can extract the length and starting location.
	;Unfortunately we broke our pointer, so must reconstruct it.
	dec bx
	mov ax, 0x20
	mul bx
	mov si, ax		;ok, so si should now point to the start of the directory entry we found.
	mov ax, word [ds:si+0x1A]	;start of the file in clusters (low word, for FAT32) is at offset 1A
	mov di, KernelClusterLocationLow
	stosw			;rely on es still being set to 0
	mov ax, word [ds:si+0x14]	;high word of file start location for FAT32. Only valid if we are on FAT32
	mov di, KernelClusterLocationHigh
	stosw
	mov ax, word [ds:si+0x1C]	;low word of the file length in bytes
	mov dx, word [ds:si+0x1e]	;high word of the file length in bytes
	mov word [fs:KernelSizeInBytesPtr], ax	;store the low word for when we need it later
	mov word [fs:KernelSizeInBytesPtr+2], dx ;same for the high word
	mov cx, BootSectorMemSectr
	mov ds, cx
	mov si, BytesPerSector
	mov bx, word [ds:si]
	div bx						;divide total number of bytes by bytes per sector, to get the length in sectors (stored in ax)
	inc ax 						;assume that there is some remainder so grab another sector to be safe
	mov di, KernelSectorLength
	stosw

	pop ebp
	ret

	_kern_not_found:
	mov si, NotFoundErrString
	sub si, 0x7E00
	call PrintString
	hlt
	jmp $-2

;Actually loads the kernel binary, based on the information we derived in FindKernelBinary.
LoadInKernel:
	push ebp
	mov ebp, esp

	xor eax, eax
	mov ax, 0x50
	mov ds, ax
	mov gs, ax
	mov ax, 0x0
	mov fs, ax										;fs points to the origin, which our local offsets are measured to.
	call F32GetRootDirLocation	
	;root dir is at cluster 2, this location is now in ax. Subtract two clusters worth from it.
	mov bx, word [gs:CustomDiskBlocksPerCluster]
	sub ax, bx
	sub ax, bx 
	mov word [fs:DiskDataBlocksOffset], ax

	mov dx, 0										;Use dx to track the number of sectors loaded
	mov ax, 0x1000									;start of write buffer
	mov ds, ax
	mov eax, dword [fs:KernelClusterLocationLow]			;FIXME - upper word ignored in FAT32!

	xor ecx, ecx

	_k_load_next_cluster:
	mov dword [gs:CurrentlyLoadingCluster], eax
	mov bx, word [gs:CustomDiskBlocksPerCluster]
	add dx, bx

	;ax is currently the location of the next cluster to load.
	;we need to convert this number to hardware blocks (easy as DiskBlocksPerCluster is already in bx),
	;and add the filesystem's offset to the result.
	mul bx
	mov bx, word [fs:DiskDataBlocksOffset]
	add ax, bx
	mov ecx, eax	;LoadDiskSectors expects this value in cx.

	push ecx
	push dx
	mov bx, word [gs:CustomDiskBlocksPerCluster]	;load 1 cluster's worth of blocks
	call LoadDiskSectors
	pop dx
	pop ecx
	mov ax, word [fs:KernelSectorLength]
	cmp dx, ax
	jz _k_load_done

	;get the next cluster address from the FAT. cx is the cluster address we just loaded, gs points to the boot sector and fs points to our data.
	;FIXME - this calculation does not work if the sectors per cluster is not 1. Missing a factor somewhere.
	; mov ax, cx 				;set ax to the (physical) cluster we just loaded
	; mov bx, word [fs:DiskDataBlocksOffset]
	; sub ax, bx				;subtract the offset to get back to the FAT index
	mov eax, dword [gs:CurrentlyLoadingCluster]
	mov bx, 4
	mul bx					;multiply the FAT entry index by 4 to get the byte offset
	mov esi, eax
	push gs
	mov ax, FATSector
	mov gs, ax				;point gs to the FAT
	mov eax, dword [gs:si]	;get the offset of the next cluster

	pop gs					;restore gs value
	and eax, 0x0FFFFFFF		;shave off upper nybble
	cmp eax, 0x0FFFFFF8		;anything of this value or above is an EOF
	jge _k_load_done

	;increment our data pointer

	push eax				;save the cluster number for later, we expect it to be in ax when we loop
	mov bx, word [gs:BytesPerSector]
	xor ax, ax
	mov al, byte [gs:SectorsPerCluster]
	mul bx
	mov bx, ds
	shr ax, 4				;shift-right the number of bytes to get a 16-bit (memory!) sector offset from a byte offset
	add ax, bx				;add one cluster's worth of bytes to the memory pointer
	mov ds, ax

	pop eax					;restore the cluster number into ax
	jmp _k_load_next_cluster

	_k_load_done:
	pop ebp
	ret

fat_load_err:
	mov si, FSErrString
	sub si, 0x7E00
	call PrintString
	hlt
	jmp $

;FIXME: aaarrgggh! These are getting clobbered by something :( 0x8245
;data
WrongFATErr					db 'FAT16 not currently supported', 0x0d, 0x0a, 0x0
FSErrString					db 'Could not load filesystem', 0x0d, 0x0a, 0
NotFoundErrString			db 'Could not locate ANDY.SYS', 0x0d, 0x0a, 0
KernelName 					db 'ANDY    SYS'
KernelNameLen 				dw 11
KernelClusterLocationLow 	dw 0	;these values are filled in by FindKernelBinary
KernelClusterLocationHigh	dw 0
KernelSectorLength			dw 0
DiskDataBlocksOffset		dw 0	;offset to "cluster 0" in hardware blocks
