;This file defines the low-memory layout used during bootstrapping the kernel
;from 16-bit to 32-bit mode, and a few static bits and bobx used later
%define IDTOffset    0x1006	;where we store the interrupt descriptor table in our data segment
%define IDTSize      0x800	;256 entries of 8 bytes each
%define IDTPtr	     0x1000	;IDT pointer block goes right before the IDT, at the start of page 1

%define FullGDT      0xe08
%define FullGDTPtr   0xe00
%define FullTSS      0x1e40

%define CursorRowPtr 0xd08	;where we store screen cursor row in kernel data segment
%define CursorColPtr 0xd09	;where we store cursor col in kernel data segment
%define DisplayMemorySeg 0x18	;segment identifier for mapped video RAM
%define TextConsoleOffset 0x18000	;text mode console memory is at 0xB8000 and the segment starts at 0xA0000
%define DefaultTextAttribute 0x07	;light-grey-on-black https://wiki.osdev.org/Printing_To_Screen

%define CurrentProcessRegisterScratch     0x1A000 ;temporary storage location for process's register state when context-switching
%define CurrentProcessRegisterScratchSize 0x90    ;this region follows the format of SavedRegisterStates32 in process.h
