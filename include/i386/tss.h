#ifndef _TSS_H
#define _TSS_H


#include "types.h"
#include "kproc.h"
#define TSS_32_TYPE 0x9

struct tss {
    uint32_t backlink;
    uint32_t *esp0;
    uint32_t ss0;
    uint32_t *esp1;
    uint32_t ss1;
    uint32_t *esp2;
    uint32_t ss2;
    uint32_t *cr3;
    uint32_t *eip ;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t *esp;
    uint32_t *ebp;//ebp和esp都是栈指针
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trace;
    uint16_t io_base;//io位图在TSS中的偏移地址
};




static inline void ltr(uint16_t sel){
  asm volatile("ltr %w0" : : "r" (sel));
}

void update_tss_esp(uint8_t *kstack);
void tss_init(void);

#endif