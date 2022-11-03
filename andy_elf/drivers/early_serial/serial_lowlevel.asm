[bits 32]

;All I/O ports are 8 bits wide
%define SERIAL_BASE         0x03F8
;these ports are changed if DLAB (Divisor Latch Access Bit) is 1
%define SERIAL_DATA         SERIAL_BASE + 0
%define SERIAL_INT_ENABLE   SERIAL_BASE + 1
;if DLAB is 1 then the first two ports are reassigned as follows
%define SERIAL_DIVISOR_LSB  SERIAL_BASE + 0
%define SERIAL_DIVISOR_MSB  SERIAL_BASE + 1

%define SERIAL_INTERRUPT_ID     SERIAL_BASE + 2
%define SERIAL_LINE_CONTROL     SERIAL_BASE + 3
%define SERIAL_MODEL_CONTROL    SERIAL_BASE + 4
%define SERIAL_LINE_STATUS      SERIAL_BASE + 5
%define SERIAL_MODEM_STATUS     SERIAL_BASE + 6
%define SERIAL_SCRATCH          SERIAL_BASE + 7

global early_serial_lowlevel_init
global early_serial_putchar

; Purpose - initialises the first serial port (COM1, ttyS0 or however you want to name it)
; This is "early", i.e. loadable before memory management etc, and is only intended for debugging
; output on the serial port.
; Hard-sets 9600 8-N-1 parameters
early_serial_lowlevel_init:
    push ebp
    mov ebp, esp
    push dx

    ;Configure the baud rate.
    ;First we need to access the divsor
    xor ax, ax
    mov dx, SERIAL_LINE_CONTROL
    in ax, dx
    or al, 0x80    ;set the MSB
    out dx, al
    ;Now, we can use SERIAL_DIVISOR_xSB
    ;we want 9600 baud, which is a divisor of 12 or 0x0C
    mov dx, SERIAL_DIVISOR_MSB
    xor ax, ax
    out dx, al
    mov dx, SERIAL_DIVISOR_LSB
    mov al, 0x0C
    out dx, al

    ;Now clear the DLAB bit. This is done by clearing the MSB of the line control register.
    xor ax, ax
    mov dx, SERIAL_LINE_CONTROL
    in al, dx
    and al, 0x7F
    ;Data bits are bits 0 and 1 of the line control register. 11 => 8 bits.
    or al, 0x03
    ;Stop bits is bit 2 of the line control register. 0 => 1 stop bit
    and al, 0xFB
    ;Parity is bits 3,4,5 of the line control register. xx0 => No parity
    and al, 0xC7
    out dx, al

    ;We don't yet have PM interrupts set up. So we can't use them.
    xor al, al
    mov dx, SERIAL_INT_ENABLE
    out dx, al

    pop dx
    pop ebp
    ret

; Purpose - writes a character to the (initialised) serial port
; This is a low-performance, "early" implementation designed to be able to operate before interrupts are available
; For this reason, it will always wait until the line is available and the char written before returning.
; Arguments: one 8-bit ASCII character to write
early_serial_putchar:
    push ebp
    mov ebp, esp
    push dx

    ;Check if the transmit buffer is empty yet
    _early_serial_putchar_wait_loop:
    mov dx, SERIAL_LINE_STATUS
    in al, dx
    test al, 0x20    ;we need to check bit 5
    jz _early_serial_putchar_wait_loop ;zero flag set => result of AND was zero => serial line is transmitting

    ;now write the char
    mov al, BYTE [ebp+8]
    mov dx, SERIAL_DATA
    out dx, al

    ;return
    pop dx
    pop ebp
    ret
