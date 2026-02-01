#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

//These handlers are all defined in interrupts.asm
void ITimer();
void IKeyboard();
void IDummy();
void ISerial2();
void ISerial1();
void IParallel2();
void IFloppy();
void ISpurious();
void ICmosRTC();
void IRQ9();
void IRQ10();
void IRQ11();
void IMouse();
void IFPU();
void IPrimaryATA();
void ISecondaryATA();

#endif
