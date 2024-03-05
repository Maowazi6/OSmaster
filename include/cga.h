#ifndef _CGA_H
#define _CGA_H
#include "types.h"

void cga_putc(uint8_t c);
void cga_clear();
void cga_putstr(char *str);
void put_int2hex(uint32_t num);

#endif