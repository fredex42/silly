[BITS 32]

section .text
global inb
global inw
global ind
global outb
global outw
global outd
global io_wait
global cli
global sti

cli:
	cli
	ret

sti:
	sti
	ret

;Purpose: reads in a byte from the given IO port and returns the value
;Expects: 16-bit integer representing the port to request
;Returns: 8-bit integer representing the value from the port
;Clobbers: eax (for return value)
inb:
	push ebp
	mov ebp, esp
	push edx

	xor edx,edx
	xor eax, eax
	mov dx, word [ebp+8]	;first argument - port number to read.
	in al, dx

	pop edx
	pop ebp
	ret

inw:
	push ebp
	mov ebp, esp
	push edx

	xor edx,edx
	xor eax, eax
	mov dx, word [ebp+8]	;first argument - port number to read.
	in ax, dx

	pop edx
	pop ebp
	ret

ind:
	push ebp
	mov ebp, esp
	push edx

	xor edx,edx
	xor eax, eax
	mov dx, word [ebp+8]	;first argument - port number to read.
	in eax, dx

	pop edx
	pop ebp
	ret

;Purpose: writes out a byte to the given IO port and returns nothing
;Expects: 16-bit integer representing the port to write to
;				8-bit integer giving the byte to write
;Returns: Nothing
;Clobbers: nothing
outb:
	push ebp
	mov ebp, esp
	push edx
	push eax

	xor edx, edx
	xor eax, eax
	mov dx, word [ebp+8]
	mov al, byte [ebp+12]
	out dx, al

	pop eax
	pop edx
	pop ebp
	ret

outw:
	push ebp
	mov ebp, esp
	push edx
	push eax

	xor edx, edx
	xor eax, eax
	mov dx, word [ebp+8]
	mov ax, word [ebp+12]
	out dx, ax

	pop eax
	pop edx
	pop ebp
	ret

outd:
	push ebp
	mov ebp, esp
	push edx
	push eax

	xor edx, edx
	xor eax, eax
	mov dx, word [ebp+8]
	mov eax, dword [ebp+12]
	out dx, eax

	pop eax
	pop edx
	pop ebp
	ret
	
;Purpose: Wait a very small amount of time (1 to 4 microseconds, generally).
; Useful for implementing a small delay for PIC remapping on old hardware or
; generally as a simple but imprecise wait.
; You can do an IO operation on any unused port: the Linux kernel by default
; uses port 0x80, which is often used during POST to log information on the
; motherboard's hex display but almost always unused after boot.
; https://wiki.osdev.org/Inline_Assembly/Examples#I.2FO_access
;Returns: 0
io_wait:
	xor eax, eax
	out 0x80, al
	ret
