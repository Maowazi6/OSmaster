#ifndef _GDT_H
#define _GDT_H

#include "types.h"

#define STA_X       0x8     //表示代码段
#define STA_W       0x2     //这个用在数据段表示读/写
#define STA_R       0x2     //这个用在代码段表示读/执行
#define TI_GDT 0
#define TI_LDT 1


#define SEG_KCODE 1  
#define SEG_KDATA 2  
#define SEG_UCODE 3  
#define SEG_UDATA 4  
#define SEG_TSS   5  
#define NSEGS     6

#define KDPL 0
#define UDPL 3

struct gdt_desc {
  uint32_t lim_15_0 : 16;  
  uint32_t base_15_0 : 16; 
  uint32_t base_23_16 : 8; 
  uint32_t type : 4;       
  uint32_t s : 1;          
  uint32_t dpl : 2;        
  uint32_t p : 1;          
  uint32_t lim_19_16 : 4;  
  uint32_t avl : 1;        
  uint32_t rsv1 : 1;       
  uint32_t db : 1;         
  uint32_t g : 1;          
  uint32_t base_31_24 : 8; 
};



static inline void lgdt(struct gdt_desc *p, int size) {
  volatile uint16_t pd[3];

  pd[0] = size-1;
  pd[1] = (uint32_t)p;
  pd[2] = (uint32_t)p >> 16;

  asm volatile("lgdt (%0)" : : "r" (pd));
}

void create_normal_desc(struct gdt_desc *normal_desc, uint32_t base, uint32_t lim, int dpl, int type);
void create_small_desc(struct gdt_desc *normal_desc, uint32_t base, uint32_t lim, int dpl, int type);

#endif