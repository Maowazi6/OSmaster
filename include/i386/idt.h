#ifndef _IDT_H
#define _IDT_H

#include "types.h"
#define STS_TRAPGATE    0xF     
#define STS_INTGATE     0xE     
#define STS_CALLGATE    0xC     
#define STS_TASKGATE    0x9

#define MAX_IDTDESC     256
#define EFLAGS_IF   0x00000200 
#define EFLAGS_MBS	(1 << 1)	// 此项必须要设置
#define EFLAGS_IOPL_3	(3 << 12)	// IOPL3,用于测试用户程序在非系统调用下进行IO
#define EFLAGS_IOPL_0	(0 << 12)	// IOPL0
#define EFLAGS_IF_1	(1 << 9)	// if为1,开中断
#define EFLAGS_IF_0	0		// if为0,关中断

struct gate_desc {
  uint32_t off_15_0 : 16;   
  uint32_t cs : 16;         
  uint32_t keep : 8;                
  uint32_t type : 4;        
  uint32_t s : 1;           
  uint32_t dpl : 2;         
  uint32_t p : 1;           
  uint32_t off_31_16 : 16;  
};

struct trap_context {
    uint32_t vec_no;	 
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;	 
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
	
    uint32_t gs;	//虽然是16位，但是高16位无效
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
	
    uint32_t errcode;		 
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp0;
    uint32_t ss;
};

struct proc_context{
   uint32_t ebp;
   uint32_t ebx;
   uint32_t edi;
   uint32_t esi;
   uint32_t eip;
};



static inline void lidt(struct gate_desc *p, int size) {
  volatile uint16_t pd[3];

  pd[0] = size-1;
  pd[1] = (uint32_t)p;
  pd[2] = (uint32_t)p >> 16;

  asm volatile("lidt (%0)" : : "r" (pd));
}


static inline void cli(void){
  asm volatile("cli");
}

static inline void sti(void){
  asm volatile("sti");
}

static inline bool get_eflag_if(void){
	volatile uint32_t eflag;
	asm volatile("pushfl; popl %0" : "=g" (eflag));
	return (EFLAGS_IF & eflag)? TRUE : FALSE;
}

void arch_disable();
void arch_enable();
bool get_arch_intr_status();
void idt_register(uint8_t vector_no, void* function);
void arch_intr_register(uint8_t vector_no, void* function);
void arch_intr_init();
#endif