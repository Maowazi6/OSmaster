#ifndef _INTERRUPT_H
#define _INTERRUPT_H

void intr_init();
bool intr_get_status();
void set_intr_noret (bool status);
bool set_intr(bool status);
void intr_disable();
void intr_enable();
void intr_register(uint8_t vector_no, void* function);
#endif