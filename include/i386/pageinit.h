#ifndef _PAGEINIT_H
#define _PAGEINIT_H
#include "types.h"
#include "mmlayout.h"

#define KPDE_NUM 			224
#define KPDE_OFF 			768

#define PDE_IDX(addr) 		(((uint32_t)addr >> 22) & 0x3ff)
#define PTE_IDX(addr) 		(((uint32_t)addr >> 12) & 0x3ff)

#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xFFF)

//第0位表示是否存在页
#define PG_P_TRUE  		    0x1
#define PG_P_FALSE  		0x0    
//第1位	RW位意为读写位。若为1表示可读可写，若为0表示可读不可写	
#define PG_RW_W	 			0x2     
//第3位为普通用户/超级用户位.若为 1 时，表示处于 User 级，任意级别（0,1,2,3）特权的程序都可以访问该页。
//若为0表示处于 Supervisor 级，特权级别为 3 的程序不允许访问该页，该页只允许特权级别为 0,1,2的程序可以访问。
#define PG_US_U	 			0x4 	
//PWT, Page-level Write-through，页写透，为1表示既写内存又写高速缓存
#define PG_PWT_UP         	0x8 
//PCD, Page-level Cache Disable,意为页级高速缓存禁止位.若为 1 表示该页启用高速缓存，为 0 表示禁止将该页缓存.这里默认设置0
#define PG_PCD_UP           0x10
//A, Accessed,意为访问位 。 若为 1 表示该页被 CPU 访问过(读或写)，由硬件自己设置
#define PG_A_UP				0x20
//D意为脏页位.当 CPU 对 一 个页面执行写操作时，就会设置对应页表项的D位为 l 。 此项
//仅针对页表项有效，并不会修改页目录项中的 D 位。
#define PG_D_UP				0x40
//PAT, Page Attribute Table ，意为页属性表位，能够在页面一级的粒度上设置内存属性。默认置为0表示4kb,为1是4MB
#define PG_PAT_UP			0x80
//G全局位，为1表示该页是全局的高速缓存一直保存，cr3被重新加载会清空TLB，置1后不会被清空，该页一直保存
#define PG_G_UP				0x100
//AVL,cpu忽略该位，留给软件使用 ，默认为0
#define PG_AVL_DEF          0x0


static inline uint32_t rcr2(void) {
  uint32_t val;
  asm volatile("movl %%cr2,%0" : "=r" (val));
  return val;
}


static inline void lcr3(uint32_t val) {
  asm volatile("movl %0,%%cr3" : : "r" (val));
}




uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void page_table_pte_remove(uint32_t vaddr);
void page_table_add(void* _vaddr, void* _page_phyaddr);
void _page_init();

void page_dir_activate(uint32_t *proc_pgdir);
uint32_t* create_page_dir(void);
void free_page_dir(uint32_t *page_dir);
#endif