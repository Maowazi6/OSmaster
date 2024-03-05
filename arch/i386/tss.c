#include "tss.h"
#include "gdt.h"
#include "types.h"
#include "string.h"
#include "kproc.h"
#include "cga.h"


struct tss tss;
extern struct gdt_desc gdt_table[NSEGS];

/* 更新tss中esp0字段的值为pthread的0级线 */
void update_tss_esp(uint8_t *kstack) {
   tss.esp0 = kstack;
}


// 在gdt中创建tss
void tss_init() {
	uint32_t tss_size = sizeof(tss);
	memset(&tss, 0, tss_size);
	tss.ss0 = ((SEG_KDATA<<3) + + TI_GDT + KDPL);
	tss.io_base = (uint16_t)0xffff;//这里设置io位图超过tss限长表示没有IO位图，所以禁止用户使用io指令
	
	create_small_desc(&gdt_table[SEG_TSS], &tss, sizeof(struct tss) - 1, KDPL, TSS_32_TYPE);
	gdt_table[SEG_TSS].s = 0;
	
	cga_putstr("tss init done");
}

