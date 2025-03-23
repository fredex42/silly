#ifndef __X86_CONTROL_REGISTERS_H
#define __X86_CONTROL_REGISTERS_H

//CR0 values
#define CR0_PE  1<<0  //1 if system is in protected mode, 0 if in real mode
#define CR0_MP  1<<1  //Controls interaction of WAIT/FWAIT instructions with TS flag in CR0
#define CR0_EM  1<<2  //If set, no x87 floating-point unit present, if clear, x87 FPU present
#define CR0_TS  1<<3  //Allows saving x87 task context upon a task switch only after x87 instruction used
#define CR0_ET  1<<4  //On the 386, it allowed to specify whether the external math coprocessor was an 80287 or 80387
#define CR0_NE  1<<5  //Enable internal x87 floating point error reporting when set, else enables PC style x87 error detection
#define CR0_WP  1<<16 //When set, the CPU can't write to read-only pages when privilege level is 0
#define CR0_AM  1<<18 //Alignment check enabled if AM set, AC flag (in EFLAGS register) set, and privilege level is 3
#define CR0_NW  1<<29 //Globally enables/disable write-through caching
#define CR0_CD  1<<30 //Globally enables/disable the memory cache
#define CR0_PG  1<<31 //If 1, enable paging and use the § CR3 register, else disable paging. 
#endif
