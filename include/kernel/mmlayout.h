#ifndef _MMLAYOUT_H
#define _MMLAYOUT_H

#define KERN_BASE 			0xc0000000
#define KERN_END			0x40000000-0x400000-0x8000000

#define USER_VADDR_START    0x80000000
#define USER_VADDR_TOP      0xc0000000
#define PAGE_BASE(addr) 	(uint32_t)addr & 0xfffff000

#define PGROUNDDOWN(a) (((a)) & ~(PAGE_SIZE-1))
#define PGROUNDUP(sz)  (((uint32_t)(sz)+PAGE_SIZE-1) & ~(PAGE_SIZE-1))

#define PAGE_SIZE 			4096

#define V2P(a) (((uint32_t) (a)) - KERN_BASE)
#define P2V(a) ((void *)(((char *) (a)) + KERN_BASE))


#endif