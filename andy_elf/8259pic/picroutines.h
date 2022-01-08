#ifndef __8259_PIC_ROUTINES_H
#define __8259_PIC_ROUTINES_H
/* Sends the End-Of-Interrupt signal to the correct PIC(s) */
void pic_send_eoi(uint8_t irq_number);
uint16_t pic_get_irq_reg(uint8_t cmd_word); //internal routine used for getting IRR or ISR
uint16_t pic_get_irr(); //gets the Interrupt Request Register for both PICs. Upper 8 bytes is PIC1 lower is PIC2.
uint16_t pic_get_isr(); //gets the In-Service Register for both PICs. Upper 8 bytes is PIC1 lower is PIC2
void disable_lapic();

#endif
