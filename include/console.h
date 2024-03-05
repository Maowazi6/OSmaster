#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H
#include "types.h"

extern struct lock console_lock;  

void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char* str);
void console_put_char(uint8_t char_asci);
void console_put_int(uint32_t num);
void sys_putchar(char char_asci);
void sys_clear();
#endif

