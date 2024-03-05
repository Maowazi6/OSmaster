#ifndef _KALLOC_H
#define _KALLOC_H
#include "types.h"
#include "mm.h"
#include "pageinit.h"
struct mem_block* arena2block(struct arena* a, uint32_t idx);
struct arena* block2arena(struct mem_block* b);
void* mempool_alloc(enum pool_flags m_pool_flag,uint32_t cnt);
void mempoll_free(enum pool_flags m_pool_flag, uint32_t *pg_phy_addr, uint32_t cnt);
void* valloc(struct virtual_addr *m_vaddr, uint32_t cnt);
void* vfree(struct virtual_addr *m_vaddr, uint32_t *vaddr,uint32_t cnt);
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr);
void* _kalloc(uint32_t cnt);
void* _ualloc(struct virtual_addr *m_vaddr, uint32_t cnt);
void* malloc_page(enum pool_flags m_flag, uint32_t page_cnt, struct virtual_addr *m_vaddr);
void _kfree(uint32_t *addr, uint32_t cnt);
void _ufree(uint32_t *vaddr, uint32_t cnt, struct virtual_addr *m_vaddr);
void mfree_page(enum pool_flags m_flag, uint32_t *addr, uint32_t cnt, struct virtual_addr *m_vaddr);
void* block_malloc(uint32_t size, struct mem_block_desc *m_desc, enum pool_flags m_flag, struct virtual_addr *m_vaddr);
void block_free(void* ptr, enum pool_flags m_flag, struct virtual_addr *m_vaddr);
void *malloc_a_kpage();
void* kalloc(uint32_t size);
void kfree(uint32_t *addr);
void* ualloc(uint32_t size, struct mem_block_desc *u_desc, struct virtual_addr *m_vaddr);
void ufree(uint32_t *addr, struct virtual_addr *u_vaddr);
void* sys_malloc(uint32_t size);

#endif