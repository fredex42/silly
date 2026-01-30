IKeyboard:	;keyboard interrupt handler
	pushf
	push bx
	xor bx, bx
	mov bl, '.'
	call PMPrintChar
	
	call PIC_Send_EOI
	pop bx
	popf
	iret
