#include "assert.h"
#include "types.h"
#include "cga.h"

void panic_spin(char* filename,	int line, const char* func, const char* condition) {
   set_intr(false);	// 因为有时候会单独调用panic_spin,所以在此处关中断。
   cga_putstr("\n\n\n!!!!! error !!!!!\n");
   cga_putstr("filename:");cga_putstr(filename);cga_putstr("\n");
   cga_putstr("line:0x");put_int2hex(line);cga_putstr("\n");
   cga_putstr("function:");cga_putstr((char*)func);cga_putstr("\n");
   cga_putstr("condition:");cga_putstr((char*)condition);cga_putstr("\n");
   while(1);
}