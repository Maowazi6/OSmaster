#include "cga.h"
#include "types.h"
#include "sync.h"
#include "stdio.h"

struct lock console_lock;    // 控制台锁

/* 初始化终端 */
void console_init() {
  lock_init(&console_lock); 
}

/* 获取终端 */
void console_acquire() {
   sync_acquire(&console_lock);
}

/* 释放终端 */
void console_release() {
   sync_release(&console_lock);
}

/* 终端中输出字符串 */
void console_put_str(char* str) {
   console_acquire(); 
   cga_putstr(str); 
   console_release();
}

/* 终端中输出字符 */
void console_put_char(uint8_t char_asci) {
   console_acquire(); 
   cga_putc(char_asci); 
   console_release();
}

/* 终端中输出16进制整数 */
void console_put_int(uint32_t num) {
   console_acquire(); 
   put_int2hex(num); 
   console_release();
}

/* 清屏 */
void console_clear() {
   console_acquire(); 
   cga_clear();
   console_release();
}

void printk(const char* format, ...) {
   va_list args;
   va_start(args, format);
   char buf[1024] = {0};
   vsprintf(buf, format, args);
   va_end(args);
   console_put_str(buf);
}


/* 向屏幕输出一个字符 */
void sys_putchar(char char_asci) {
   console_put_char(char_asci);
}

void sys_clear(){
	console_clear();
}