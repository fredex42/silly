#include <types.h>

#ifndef __X86_SETUP_IDT_H__
#define __X86_SETUP_IDT_H__

void setup_idt();

extern void IDivZero();
extern void IDebug();
extern void INMI();
extern void IBreakPoint();
extern void IOverflow();
extern void IBoundRange();
extern void IOpcodeInval();
extern void IDevNotAvail();
extern void IDoubleFault();
extern void IReserved();
extern void IInvalidTSS();
extern void ISegNotPresent();
extern void IStackSegFault();
extern void IGPF();
extern void IPageFault();
extern void IFloatingPointExcept();
extern void IAlignmentCheck();
extern void IMachineCheck();
extern void ISIMDFPExcept();
extern void IVirtExcept();
extern void ISecurityExcept();

#endif // __X86_SETUP_IDT_H__