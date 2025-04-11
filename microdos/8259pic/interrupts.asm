[BITS 32]

;function exports
global ISpurious
global ITimer
global IDummy
global ISerial1
global ISerial2
global IParallel2
global IParallel1
global IFloppy1
global IRQ9
global IRQ10
global IRQ11
global IMouse
global IFPU
global IPrimaryATA
global ISecondaryATA

;function imports
extern pic_send_eoi
extern pic_get_irr
extern pic_get_isr
extern PMPrintChar


ITimer:     ;timer interrupt handler
  pushf
  push bx
  xor bx, bx

  mov ebx, 0
  push ebx
	call pic_send_eoi
  add esp, 4

  pop bx
  popf
  iret

ISerial2:             ;IRQ3 serial port 2 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 3
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

ISerial1:             ;IRQ4 serial port 1 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 4
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IParallel2:             ;IRQ5 parallel port 2 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 5
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IFloppy1:             ;IRQ6 floppy disk 1 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 6
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

ISpurious:  ;IRQ7 spurious interrupt handler (also parallel port 1)
  pushf
  push ax

  call pic_get_isr
  shr ax, 8       ;upper 8 bits are PIC1
  and ax, 0x40    ;check bit 7
  jz no_eoi_reqd  ;if result is 0 then this is a suprious IRQ and no EOI is required

  mov ax, 7
  push ax
  call pic_send_eoi
  add esp, 4

  no_eoi_reqd:
  pop ax
  popf
  iret

IRQ9:             ;IRQ9
  pushf
  push bx
  mov ebx, 9
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IRQ10:             ;IRQ10
  pushf
  push bx
  mov ebx, 10
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IRQ11:             ;IRQ11
  pushf
  push bx
  mov ebx, 11
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IMouse:             ;IRQ12 PS/2 mouse
  pushf
  push bx
  mov ebx, 12
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IFPU:             ;IRQ13 FPU/Coprocessor/inter-processor
  pushf
  push bx
  mov ebx, 13
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

extern ata_service_interrupt  ;this is defined in the ata_pio driver for servicing IDE interrupts

IPrimaryATA:             ;IRQ14 Primary IDE
  pushf
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  
  mov ebx, 0
  push bx
  ;call ata_service_interrupt  ;tell the ATA driver that this cane from bus 0 (primary)
  add esp, 2

  mov ebx, 14
  push ebx
  call pic_send_eoi
  add esp, 4

  pop ebp
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  popf
  iret

ISecondaryATA:             ;IRQ15 Secondary IDE
  pushf
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp

  mov ebx, 1
  push bx
  ;call ata_service_interrupt ;tell the ATA driver that this cane from bus 1 (secondary)
  add esp, 2

  mov ebx, 15
  push ebx
  call pic_send_eoi
  add esp, 4

  pop ebp
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  popf
  iret

IDummy:   ;generic dummy interrupt handler
  mov ebx, 2
  push ebx
  call pic_send_eoi
  add esp, 4
  iret
