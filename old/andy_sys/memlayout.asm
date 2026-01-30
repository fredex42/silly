%define IDTOffset    0x00	;where we store the interrupt descriptor table in our data segment
%define IDTSize      0x800	;256 entries of 8 bytes each
%define IDTPtr	     0x800	;IDT pointer block goes straight after the IDT
%define TSS_Selector 0x20	;TSS starts at offset 0 in this selector
%define CursorRowPtr 0x900	;where we store screen cursor row in kernel data segment
%define CursorColPtr 0x901	;where we store cursor col in kernel data segment
%define DisplayMemorySeg 0x18	;segment identifier for mapped video RAM
%define TextConsoleOffset 0x18000	;text mode console memory is at 0xB8000 and the segment starts at 0xA0000
%define DefaultTextAttribute 0x07	;light-grey-on-black https://wiki.osdev.org/Printing_To_Screen
%define StringTableOffset 0x1000	;offset to the string table in our data segment. The string data above is copied to this location once in PM
