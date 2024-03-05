#include "kalloc.h"
#include "pageinit.h"
#include "mm.h"
#include "mmlayout.h"
#include "general.h"
#include "string.h"
#include "assert.h"
#include "sync.h"

extern struct mem_block_desc k_block_descs[DESC_CNT];	//内存描述符类似buddy
extern struct pool kernel_pool, user_pool;      		//用户和内核内存池
extern struct virtual_addr kernel_vaddr;	 			//内核线性地址


/* 返回arena中第idx个内存块的地址 */
struct mem_block* arena2block(struct arena* a, uint32_t idx) {
  return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

/* 返回内存块b所在的arena地址 */
struct arena* block2arena(struct mem_block* b) {
   return (struct arena*)((uint32_t)b & 0xfffff000);
}

//内核是直接映射，页表已经存在了，所以直接使用不需要建立映射
//所以申请和释放就是直接在位图里面设置0和1就行了
void* mempool_alloc(enum pool_flags m_pool_flag,uint32_t cnt) {
	int bit_idx;
	struct pool *m_pool;
	if(m_pool_flag == PF_KERNEL){
		m_pool = &kernel_pool;
	}else if(m_pool_flag == PF_USER){
		m_pool = &user_pool;
	}
	bit_idx = bitmap_scan(&(m_pool->pool_bitmap), cnt);
	if (bit_idx == -1 ) {
		return NULL;
	}
	uint32_t page_phyaddr = ((bit_idx * PAGE_SIZE) + m_pool->phy_addr_start);
	for(int i = 0;i < cnt;i++,bit_idx++){
		bitmap_set(&(m_pool->pool_bitmap), bit_idx, 1);	
	}

	return (void*)page_phyaddr;
}

void mempoll_free(enum pool_flags m_pool_flag, uint32_t *pg_phy_addr, uint32_t cnt) {
	struct pool *m_pool;
	uint32_t bit_idx = 0;
	if(m_pool_flag == PF_KERNEL){
		m_pool = &kernel_pool;
	}else if(m_pool_flag == PF_USER){
		m_pool = &user_pool;
	}
	bit_idx = ((uint32_t)pg_phy_addr - m_pool->phy_addr_start) / PAGE_SIZE;
	for(int i = 0;i < cnt;i++,bit_idx++){
		bitmap_set(&(m_pool->pool_bitmap), bit_idx, 0);
	}
}




//供用户使用，在用户虚拟地址空间位图里面申请虚拟内存
void* valloc(struct virtual_addr *m_vaddr, uint32_t cnt){
	int bit_idx;
	//cga_putstr("vaddr_bitmap\n");put_int2hex(&(m_vaddr->vaddr_bitmap));
	bit_idx = bitmap_scan(&(m_vaddr->vaddr_bitmap), cnt);
	if(bit_idx == -1 ){
		return NULL;
	}
	uint32_t page_vaddr = ((bit_idx * PAGE_SIZE) + m_vaddr->vaddr_start);
	for(int i = 0;i < cnt;i++,bit_idx++){
		bitmap_set(&(m_vaddr->vaddr_bitmap), bit_idx, 1);	
	}
	
	return (void*)page_vaddr;
}

//供用户使用，在用户虚拟地址空间位图里面释放虚拟内存
void* vfree(struct virtual_addr *m_vaddr, uint32_t *vaddr,uint32_t cnt){
	int bit_idx = ((uint32_t)vaddr - m_vaddr->vaddr_start) / PAGE_SIZE;
	for(int i = 0;i<cnt;i++,bit_idx++){
		bitmap_set(&(m_vaddr->vaddr_bitmap), bit_idx, 0);
	}
}

//fork过程中拷贝父进程的虚拟地址的bitmap并不需要再次设置vaddr的bitmap,4kb
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr) {
   struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
   sync_acquire(&mem_pool->lock);
   void* page_phyaddr = mempool_alloc(PF_USER,1);
   if (page_phyaddr == NULL) {
      sync_release(&mem_pool->lock);
      return NULL;
   }
   page_table_add(vaddr, page_phyaddr); 
   sync_release(&mem_pool->lock);
   return (void*)vaddr;
}



//分配n个页
void* _kalloc(uint32_t cnt){
	return P2V(mempool_alloc(PF_KERNEL, cnt));
}

void* _ualloc(struct virtual_addr *m_vaddr, uint32_t cnt){
	uint32_t *paddr = (uint32_t*)mempool_alloc(PF_USER, cnt);
	uint32_t *vaddr = (uint32_t*)valloc(m_vaddr, cnt);
	
	if(paddr == -1 || vaddr == -1){
		return NULL;
	}
	uint32_t *result = vaddr;
	for(int i = 0;i < cnt;i++){
		page_table_add(vaddr, paddr); //_ualloc依赖于不同架构下的页映射接口，但页映射函数并不依赖于kalloc模块，kalloc只是一个对外接口
		paddr += PAGE_SIZE / sizeof(uint32_t);
		vaddr += PAGE_SIZE / sizeof(uint32_t);
	}
	
	
	return (void*)result;
}

void *malloc_page(enum pool_flags m_flag, uint32_t page_cnt, struct virtual_addr *m_vaddr){
	uint32_t *result;
	if(m_flag == PF_KERNEL){
		result = _kalloc(page_cnt);
	}else if(m_flag == PF_USER){
		result = _ualloc(m_vaddr, page_cnt);
	}
	return result;
}



void _kfree(uint32_t *addr, uint32_t cnt){
	mempoll_free(PF_KERNEL, addr, cnt);
}

void _ufree(uint32_t *vaddr, uint32_t cnt, struct virtual_addr *m_vaddr){
	uint32_t *_vaddr = vaddr;
	uint32_t *m_paddr;
	for(int i = 0;i<cnt;i++){
		m_paddr = (uint32_t *)addr_v2p(_vaddr);
		mempoll_free(PF_USER, m_paddr, 1);
		page_table_pte_remove((uint32_t)_vaddr);
		_vaddr += PAGE_SIZE / sizeof(uint32_t);
	}
	vfree(m_vaddr, vaddr, cnt);
	
}

void mfree_page(enum pool_flags m_flag, uint32_t *addr, uint32_t cnt, struct virtual_addr *m_vaddr){
	if(m_flag == PF_KERNEL){
		_kfree(V2P(addr), cnt);
	}else if(m_flag == PF_USER){
		_ufree(addr, cnt, m_vaddr);
	}
}

//这个mem_block和m_addr 是进程部分使用 内存模块部分必须实现的接口
void* block_malloc(uint32_t size, struct mem_block_desc *m_desc, enum pool_flags m_flag, struct virtual_addr *m_vaddr) {
	struct pool* mem_pool;
	uint32_t pool_size;
	struct mem_block_desc *descs;
	descs = m_desc;
	if(m_flag == PF_KERNEL){
		mem_pool = &kernel_pool;
	}else if(m_flag == PF_USER){
		mem_pool = &user_pool;
	}
	
	pool_size = mem_pool->pool_size;

	/* 若申请的内存不在内存池容量范围内则直接返回NULL */
	if (!(size > 0 && size < pool_size)) {
		return NULL;
	}
	struct arena* a;
	struct mem_block* b;	
	sync_acquire(&mem_pool->lock);
/* 超过最大内存块1024, 就分配页框 */
	if (size > 1024) {
		uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PAGE_SIZE);    // 向上取整需要的页框数  4096是两个页框
		a = malloc_page(m_flag, page_cnt, m_vaddr);
		if (a != NULL) {
			memset(a, 0, page_cnt * PAGE_SIZE);	 // 将分配的内存清0  

		/* 对于分配的大块页框,将desc置为NULL, cnt置为页框数,large置为true */
			a->desc = NULL;
			a->cnt = page_cnt;
			a->large = true;
			sync_release(&mem_pool->lock);
			return (void*)(a + 1);		 // 跨过arena大小，把剩下的内存返回
		}else{ 
			sync_release(&mem_pool->lock);
			return NULL; 
		}
	}else{    // 若申请的内存小于等于1024,可在各种规格的mem_block_desc中去适配
		uint8_t desc_idx;
      
      /* 从内存块描述符中匹配合适的内存块规格 */
		for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
			if (size <= descs[desc_idx].block_size) {  // 从小往大后,找到后退出
				break;
			}
		}
   	       
		if (list_empty(&descs[desc_idx].free_list)) {
			a = malloc_page(m_flag, 1, m_vaddr);       // 分配1页框做为arena
			if (a == NULL) {
				sync_release(&mem_pool->lock);
				return NULL;
			}
			memset(a, 0, PAGE_SIZE);
	
			a->desc = &descs[desc_idx];
			a->large = false;
			a->cnt = descs[desc_idx].blocks_per_arena;
			uint32_t block_idx;
	
			bool old_status = set_intr(false);
	
		
			for (block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
				b = arena2block(a, block_idx);
				ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
				list_append(&a->desc->free_list, &b->free_elem);	
			}
			set_intr(old_status);
		}    
	
		b = elem2entry(struct mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)));
		memset(b, 0, descs[desc_idx].block_size);
	
		a = block2arena(b);  
		a->cnt--;		   
		sync_release(&mem_pool->lock);
		return (void*)b;
	}
}


/* 回收内存ptr */
void block_free(void* ptr, enum pool_flags m_flag, struct virtual_addr *m_vaddr){
	ASSERT(ptr != NULL);
	if (ptr != NULL) {
		struct pool* mem_pool;

		if(m_flag == PF_KERNEL){
			mem_pool = &kernel_pool;
		}else if(m_flag == PF_USER){
			mem_pool = &user_pool;
		}
	
		sync_acquire(&mem_pool->lock);   
		struct mem_block* b = ptr;
		struct arena* a = block2arena(b);	     // 把mem_block转换成arena,获取元信息
		ASSERT(a->large == 0 || a->large == 1);
		if (a->desc == NULL && a->large == true) { // 大于1024的内存
			mfree_page(m_flag, a, a->cnt, m_vaddr); 
		}else{// 小于等于1024的内存块
			list_append(&a->desc->free_list, &b->free_elem);
		
			/* 再判断此arena中的内存块是否都是空闲,如果是就释放arena */
			if (++a->cnt == a->desc->blocks_per_arena) {
				uint32_t block_idx;
				for (block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
					struct mem_block*  b = arena2block(a, block_idx);
					ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
					list_remove(&b->free_elem);
				}
				mfree_page(m_flag, a, 1, m_vaddr); 
			} 
		}   
		sync_release(&mem_pool->lock); 
   }
}

//这个后期得改，大页的申请回造成大的内存碎片，而小的内存块却不会，所以在申请大页时，就用小内存块来存储元信息
void *malloc_a_kpage(){
	uint32_t *result;
	sync_acquire(&kernel_pool.lock);
	result = _kalloc(1);
	sync_release(&kernel_pool.lock); 
	return result;
}

void *free_a_kpage(uint32_t *addr){
	sync_acquire(&kernel_pool.lock);
	_kfree(addr,1);
	sync_release(&kernel_pool.lock); 
}


//这两个实现同步访问
void* kalloc(uint32_t size){
	block_malloc(size, k_block_descs, PF_KERNEL, NULL);
}

void kfree(uint32_t *addr){
	block_free(addr, PF_KERNEL, NULL);
}

void* ualloc(uint32_t size, struct mem_block_desc *u_desc, struct virtual_addr *m_vaddr){
	block_malloc(size, u_desc, PF_USER, m_vaddr);
}

void ufree(uint32_t *addr, struct virtual_addr *u_vaddr){
	block_free(addr, PF_USER, u_vaddr);
}


//为了使栈连续，一般将用户栈放在用户虚拟内存顶部,方便以后扩展这个栈,也便于和堆区分
void* ualloc_stack(uint32_t stack_top, struct virtual_addr *m_vaddr, int32_t cnt){
	void* page_phyaddr;
	uint32_t bit_idx = ((stack_top - m_vaddr->vaddr_start) / PAGE_SIZE) - (cnt-1) -1;
	uint32_t stack_bottom = stack_top - (cnt * PAGE_SIZE);
	ASSERT(bit_idx >= 0);
	sync_acquire(&user_pool.lock);
	for(int i = 0;i<cnt;i++){
		bitmap_set(&m_vaddr->vaddr_bitmap, bit_idx, 1);
		page_phyaddr = mempool_alloc(PF_USER,1);
		if (page_phyaddr == NULL) {
			sync_release(&user_pool.lock);
			return NULL;
		}
		page_table_add((void*)stack_bottom, page_phyaddr);
		stack_bottom += PAGE_SIZE;
		bit_idx++;
	}
   
	sync_release(&user_pool.lock);
	return (void*)(stack_top);
}

void* ualloc_with_vaddr(uint32_t vaddr){
	void* page_phyaddr;
	sync_acquire(&user_pool.lock);
	page_phyaddr = mempool_alloc(PF_USER,1);
	page_table_add((void*)vaddr, page_phyaddr);
	sync_release(&user_pool.lock);
	return (void*)vaddr;
}