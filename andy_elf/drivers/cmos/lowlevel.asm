[BITS 32]

%define CMOS_REG_SELECT     0x70
%define CMOS_DATA           0x71

;Low-level interfaces to CMOS and RTC
global cmos_read_rtc_raw        ;NOTE! this is an internal method which can return inconsistent results. It's used by a higher-level method that corrects this behaviour.
global cmos_get_update_in_progress

global cmos_read
global cmos_write
global ICmosRTC

;Interrupt handler functions
extern pic_send_eoi

_cmos_read_delay:
    push ebp
    mov ebp, esp

    push ecx
    xor ecx, ecx

    .delay_loop:
    inc ecx
    cmp ecx, 0x500  ;TODO - any good way of determining this?
    jnz .delay_loop

    pop ecx

    pop ebp
    ret

;Purpose: reads a byte from the given register. C calling convention.
;Argument 1: CMOS register ID (byte)
;Return value: result (byte)
cmos_read:
    push ebp
    mov ebp, esp

    mov eax, dword [ebp + 8]
    and al, 0x7F        ;bit 7 must be clear, setting this disables NMI
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA

    and eax, 0xFF       ;we only want the byte returned
    pop ebp
    ret

;Purpose: writes a byte into the given register. C calling convention.
; CAUTION CAUTION CAUTION don't write outside RTC registers or you may screw up the bios checksum. See https://wiki.osdev.org/CMOS.
;Argument 1: CMOS register ID (byte)
;Argument 2: Value to write (byte)
cmos_write:
    push ebp
    mov ebp, esp

    mov eax, dword [ebp + 8]
    and al, 0x7F       ;bit 7 must be clear, setting it disables NMI
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    mov eax, dword [ebp + 12]
    out CMOS_DATA, al
    pop ebp
    ret

;Purpose: Retrieve the update-in-progress flag from status register A
;Arguments: None
;Return value: 1 if update is in progress, 0 otherwise
cmos_get_update_in_progress:
    push ebp
    mov ebp, esp

    ;update flag is in bit 7 of status register A
    xor eax, eax
    mov al, 0x0A
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    and al, 0x80    ;only interested in bit 7
    shl al, 0x07    ;shift it left so value is 1/0

    pop ebp
    ret

;Purpose: Read all of the CMOS registers into the given data structure
; Argument 1: Pointer to a RealTimeClockRawData structure
cmos_read_rtc_raw:
    push ebp
    mov ebp, esp

    cli
    push edi
    mov edi, dword [ebp + 8]
    xor eax, eax

    ;register 0: seconds
    mov al, 0x80    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi], al

    ;register 2: minutes
    mov al, 0x82    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+1], al

    ;register 4: hours
    mov al, 0x84    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+2], al

    ;register 6: day of week
    mov al, 0x86    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+3], al

    ;register 7: day of month
    mov al, 0x87    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+4], al

    ;register 8: month
    mov al, 0x88    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+5], al

    ;register 9: year
    mov al, 0x89    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+6], al

    ;register 32: century (maybe)
    mov al, 0xB2    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+7], al

    ;register 0a: status A
    mov al, 0x8a    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+8], al

    ;register 0b: minutes
    mov al, 0x8b    ;set bit 7, doing the read with NMI disabled
    out CMOS_REG_SELECT, al
    call _cmos_read_delay
    in al, CMOS_DATA
    mov byte [edi+9], al

    mov al, 0
    out CMOS_REG_SELECT, al ;clear bit 7 to re-enable NMI

    sti
    pop ebp
    ret

;Purpose: Safely enable IRQ 8, ensuring that NMI and interrupts are disabled while doing so
cmos_init_rtc_interrupt:
    push ebp
    mov ebp, esp

    cli
    ;first we need to set  interrupt rate on register A https://web.archive.org/web/20150514082645/http://www.nondot.org/sabre/os/files/MiscHW/RealtimeClockFAQ.txt
    mov al,     
    ;now enable the interrupt by setting bits on status register B
    mov al, 0x8B
    out CMOS_REG_SELECT, al ;select status register B and disable NMI
    call _cmos_read_delay
    in al, CMOS_DATA        ;get current value of register B
    or al, 0x40             ;set bit 6 to enable the interrupt
    push ax
    mov al, 0x8B
    out CMOS_REG_SELECT, al ;select status register B again
    pop ax
    out CMOS_DATA, al       ;write the new value
    mov byte [cmos_initialised], 1
    sti

    pop ebp
    ret

;Purpose: Handler for the RTC interrupt
ICmosRTC:             ;IRQ8 CMOS RTC interrupt handler. 
  pushf
  push eax

  ;We must read status register C both to know what happened, and to clear the processing flag
  mov ax, 0x0C      ;select status register C
  out CMOS_REG_SELECT, al
  call _cmos_read_delay
  in al, CMOS_DATA  ;read status C value into al. Bit 6 => periodic interrupt, bit 5=> alarm interrupt, bit 4=> update-ended interrupt

  test al, 0x40
  jnz .periodic
  .periodic_done:
  test al, 0x20
  jnz .alarm
  .alarm_done:
  test al, 0x10
  jnz .rtc_updated
  jmp .cmos_rtc_done

  .periodic:    ;actions for the "periodic interrupt" (we use this as a tick counter)
  inc dword [cmos_tick_counter]
  jmp .periodic_done

  .alarm:
  nop           ;not implemented yet
  jmp .alarm_done

  .rtc_updated:
  nop           ;not implemented yet

  .cmos_rtc_done:
  push ebx
  mov ebx, 8
  push ebx
  call pic_send_eoi
  add esp, 4

  pop ebx
  pop eax
  popf
  iret

section .data
    cmos_initialised:    db 0
    cmos_tick_counter:   dq 0
