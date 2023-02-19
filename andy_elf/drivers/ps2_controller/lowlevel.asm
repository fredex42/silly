[BITS 32]

;function imports
extern pic_send_eoi
extern PMPrintChar  ;temporary

;function exports
global ps2_lowlevel_init
global ps2_wait_for_clear_inputbuffer
global ps2_wait_for_data
global ps2_wait_for_data_or_timeout
global ps2_read
global ps2_read_status
global ps2_send
global IKeyboard
global ps2_disable_interrupts
global ps2_enable_interrupts

%define PS2_CMD     0x64    ;port 0x64 is CMD for output to h/w and STATUS for input to cpu
%define PS2_STATUS  0x64
%define PS2_DATA    0x60

;Purpose - polls the controller status until the input buffer is clear
ps2_wait_for_clear_inputbuffer:
    push ebp
    mov ebp, esp

    .ps2_input_not_clear:
    in al, PS2_STATUS
    test al, 0x02           ;check if bit 1 is set
    jnz .ps2_input_not_clear

    pop ebp
    ret

;Purpose - polls the controller status until the output buffer is full
ps2_wait_for_data:
    push ebp
    mov ebp, esp

    .ps2_output_is_clear:
    in al, PS2_STATUS
    test al, 0x01           ;check if bit 0 is set
    jz .ps2_output_is_clear ;loop if it's 0

    pop ebp
    ret

;Purpose - waits for either data to be present or for a certain number of iterations to pass
; Argument one: maximum number of iterations (dword)
; Return value: 0 if data present, 1 if timeout
ps2_wait_for_data_or_timeout:
    push ebp
    mov ebp, esp

    mov ecx, dword [ebp + 8]
    .ps2_output_is_clear_t:
    in al, PS2_STATUS
    test al, 0x01
    jnz .ps2_output_is_set_t
    dec ecx
    test ecx, ecx
    jnz .ps2_output_is_clear_t  ;loop if we have not got to 0
    ;if we get to here, we ran out of iterations
    mov eax, 0x01
    pop ebp
    ret

    .ps2_output_is_set_t: ;we got output
    xor eax, eax
    pop ebp
    ret

;Purpose - read a byte from the data port.
; Note that no check is made if there is any new data there, this must be done seperately.
ps2_read:
    push ebp
    mov ebp, esp

    xor eax, eax
    in al, PS2_DATA

    pop ebp
    ret

ps2_read_status:
    push ebp
    mov ebp, esp

    xor eax, eax
    in al, PS2_STATUS

    pop ebp
    ret
    
ps2_disable_interrupts:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov al, 0x20
    out PS2_CMD, al       ;read config byte
    call ps2_wait_for_data
    in al, PS2_DATA
    and al, 0xBC          ;disable bits 0, 1 & 6

    push ax
    mov al, 0x60
    out PS2_CMD, al       ;write config byte
    call ps2_wait_for_clear_inputbuffer
    pop ax
    out PS2_DATA, al

    pop ebp
    ret

ps2_enable_interrupts:
    push ebp
    mov ebp, esp

    xor eax, eax
    mov al, 0x20
    out PS2_CMD, al       ;read config byte
    call ps2_wait_for_data
    in al, PS2_DATA
    or al, 0x43          ;enable bits 0, 1 & 6

    push ax
    mov al, 0x60
    out PS2_CMD, al       ;write config byte
    call ps2_wait_for_clear_inputbuffer
    pop ax
    out PS2_DATA, al

    pop ebp
    ret

;Purpose - initialise the PS2 controller. See https://wiki.osdev.org/%228042%22_PS/2_Controller#Initialising_the_PS.2F2_Controller
;NOTE you should check if it actually exists before calling this! Otherwise there might be a lockup
;Return codes:
; The return code is a 32-bit bitfield. Bits 0-2 give the number of channels present. Bits 24-32 give the self-test status
; of channel 2 and bits 16-24 give the self-test status of channel 1.  If anything in bits 8-16 is set then there was an initialisation error.
; (bit 8 set => controller self-test failure)
ps2_lowlevel_init:
    push ebp
    mov ebp, esp

    ;Step one - disable all devices
    mov al, 0xAD
    out PS2_CMD, al   ;disable device 0
    call ps2_wait_for_clear_inputbuffer
    mov al, 0xA7
    out PS2_CMD, al   ;disable device 1
    call ps2_wait_for_clear_inputbuffer

    ;Step two - flush the output buffer by reading any pending data
    in al, PS2_DATA 

    ;Step three - disable IRQs and translation
    mov al, 0x20        ;read controller config byte
    out PS2_CMD, al
    call ps2_wait_for_data
    in al, PS2_DATA
    mov dx, ax
    and dl, 0xDC        ;clear bits 0, 1 and 6
    mov al, 0x60        ;write controller config byte
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer
    mov ax, dx
    out PS2_DATA, al

    ;Step five - perform self-test
    mov al, 0xAA        ;run self-test
    out PS2_CMD, al
    call ps2_wait_for_data
    in al, PS2_DATA
    cmp al, 0x55        ;expected value
    jnz .ps2_selftest_fail
    ;Step five A - restore the config byte, just in case it got reset
    mov al, 0x60
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer
    mov ax, dx          ;config byte is still in dx, from above
    out PS2_DATA, al     ;write the config byte

    ;Step six - determine if there are 2 channels
    mov al, 0xA8        ;enable second channel
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer
    mov al, 0x20        ;read controller config byte
    out PS2_CMD, al
    call ps2_wait_for_data
    in al, PS2_DATA
    mov bx, 0x01
    test al, 0x10       ;check if bit 5 is set. if it's clear we have two channels
    jz .ps2_lowlevel_init_step7 ;bit was clear => channel didn't enable => channel is not present
    mov bx, 0x02        ;we do have two channels if we didn't jump, so update bx

    .ps2_lowlevel_init_step7:
    ;Step 7 - check port 1 interface
    ;Port check results in dl for port 1 and dh for port 2
    ;Result code: 0x00 test passed 0x01 clock line stuck low 0x02 clock line stuck high 0x03 data line stuck low 0x04 data line stuck high 
    xor dx, dx          ;use dx to store the status. 0 => all good.
    mov al, 0xAB        ;test first ps/2 port
    out PS2_CMD, al
    call ps2_wait_for_data
    in al, PS2_DATA
    mov dl, al          ;store port 1 in lower byte of dx

    ;Step 8 - check port 2 interface
    mov dh, 0xFF
    cmp bx, 0x02
    jnz .ps2_lowlevel_init_step9    ;if we don't have two ports then skip the test
    mov al, 0xA9
    out PS2_CMD, al
    call ps2_wait_for_data
    in al, PS2_DATA
    mov dh, al          ;store port 2 in upper byte of dx

    .ps2_lowlevel_init_step9:
    ;before enabling, get the configuration byte again. We are going to selectively add interrupt-enable flags to it as we switch the ports on.
    xor cx, cx
    mov al, 0x20                    ;read config byte
    out PS2_CMD, al
    call ps2_wait_for_data
    in al, PS2_DATA
    mov cl, al                      ;put it into cx

    ;Step 9: enable port 1 if it works.
    test dl, dl
    jnz .ps2_lowlevel_init_step10    ;if test return for port 0 was not 0 then don't enable
    mov al, 0xAE                    ;enable channel 1
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer
    or cl, 0x11                     ;set bit 0 (port 1 interrupt) and bit 4 (port 1 clock)

    .ps2_lowlevel_init_step10:
    ;Step 10: enable port 1 if it works.
    test dh, dh
    jnz .ps2_lowlevel_init_step11
    mov al, 0xA8                    ;enable channel 2
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer
    or cl, 0x22                     ;set bit 1 (port 2 interrupt) and bit 5 (port 2 clock)

    .ps2_lowlevel_init_step11:
    ;Step 11: write the updated config byte
    mov al, 0x60
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer
    mov al, cl
    out PS2_DATA, al                 ;write the new config byte
    
    ;Step 12: build return code.
    mov eax, edx
    shl eax, 0x10       ;shift test status bytes into the upper half of the register
    mov al, bl          ;set the channel count as the last byte of the register
    jmp .ps2_lowlevel_init_done

    .ps2_selftest_fail:
    mov ax, 0x100
    .ps2_lowlevel_init_done:
    pop ebp
    ret

;Purpose - sends an instruction to the given channel of the controller and read the resulting
; Argument one - channel to send to (1 or 2)
; Argument two - command byte to send
; Return value - response byte from the command
ps2_send:
    push ebp
    mov ebp, esp

    mov eax, dword [ebp + 8]
    cmp al, 0x02
    jnz .ps2_do_send

    ;channel 2 requested, tell controller to send to ch. 2
    xor eax, eax
    mov al, 0xD4
    out PS2_CMD, al
    call ps2_wait_for_clear_inputbuffer

    .ps2_do_send:
    xor eax, eax
    mov eax, dword [ebp + 12]
    out PS2_DATA, al
    call ps2_wait_for_data
    in al, PS2_DATA

    pop ebp
    ret

IKeyboard:	;keyboard interrupt handler
	pushf
	push ax
    push bx
    in al, PS2_DATA ;get the current character from the buffer. If the buffer is not flushed then no more interrupts occur.

	xor bx, bx
	mov bl, '.'
	call PMPrintChar

    mov ebx, 1
    push ebx
    call pic_send_eoi
    add esp, 4

	pop bx
    pop ax
	popf
	iret
