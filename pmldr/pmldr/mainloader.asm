[BITS 32]

;The prologue code should have put drive info here. See http://www.ctyme.com/intr/rb-0715.htm for details of the table.
%define DriveParamsBuffer 0x500

;The prologue jumps us to here when we are in protected mode.
_pm_start:
    hlt
    jmp $