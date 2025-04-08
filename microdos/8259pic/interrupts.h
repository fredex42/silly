#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

//Standard ISA irq handlers
void ITimer();
void ISerial2();
void ISerial1();
void IParallel2();
void IFloppy1();
void ISpurious();
void IRQ9();
void IRQ10();
void IRQ11();
void IMouse();
void IFPU();
void IPrimaryATA();
void ISecondaryATA();
void IDummy();

#endif
