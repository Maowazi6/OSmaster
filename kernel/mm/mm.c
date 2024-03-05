#include "mm.h"
#include "mmlayout.h"
#include "cga.h"
#include "bitmap.h"
#include "sync.h"
#include "string.h"
#include "assert.h"
#include "general.h"

extern void _page_init();
extern char end[];  
struct mem_block_desc k_block_descs[DESC_CNT];	//内存描述符类似buddy
struct pool kernel_pool, user_pool;      		//用户和内核内存池
struct virtual_addr kernel_vaddr;	 			//内核线性地址


//初始化内存池和内核线性地址
static void mem_pool_init(uint32_t global_memsize) {
	cga_putstr("   mem_pool_init start\n");
	uint32_t page_table_size = PAGE_SIZE * 225;	  //从769~992为内核直接内存区域，后32项PDE有128MB映射区域暂时保留，后续作为高端内存 也就是224个pde+1个pagedir
	uint32_t end_roundup = PGROUNDUP((uint32_t)V2P(end));
	
	uint32_t used_mem = (end_roundup + page_table_size + MEM_BITMAP_PAGESIZE * PAGE_SIZE);	  //也就是内核代码段结束的地方到页表结束的地方是用过的内存
	uint32_t free_mem = global_memsize - used_mem;		//这里不需要对齐页不需要roundup
	uint16_t all_free_pages = free_mem / PAGE_SIZE;		  
								
	uint16_t kernel_free_pages = all_free_pages / 2;		//这里的话可分配内存就是内核和用户一人一半
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;
	
	uint32_t kbm_length = kernel_free_pages / 8;			  
	uint32_t ubm_length = user_free_pages / 8;			  
	
	uint32_t kp_start = used_mem;				  
	uint32_t up_start = kp_start + kernel_free_pages * PAGE_SIZE;	  
	
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start   = up_start;
	
	kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
	user_pool.pool_size	 = user_free_pages * PAGE_SIZE;
	
	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len	  = ubm_length;
	
	kernel_pool.pool_bitmap.bits = P2V((void*)(page_table_size + end_roundup + PKERNEL_BITMAP_OFF * PAGE_SIZE));
									
	user_pool.pool_bitmap.bits = P2V((void*)(page_table_size + end_roundup + PUSER_BITMAP_OFF * PAGE_SIZE));
	
	cga_putstr(" kernel_pool_bitmap_start:");put_int2hex((int)kernel_pool.pool_bitmap.bits);
	cga_putstr(" 	kernel_pool_phy_addr_start:");put_int2hex(kernel_pool.phy_addr_start);
	cga_putstr(" 	kernel_pool_size:");put_int2hex(kernel_pool.pool_size);
	cga_putstr("\n");
	cga_putstr(" user_pool_bitmap_start:");put_int2hex((int)user_pool.pool_bitmap.bits);
	cga_putstr(" 	user_pool_phy_addr_start:");put_int2hex(user_pool.phy_addr_start);
	cga_putstr(" 	user_pool_phy_size:");put_int2hex(user_pool.pool_size);
	cga_putstr("\n");
	
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);
	
	lock_init(&kernel_pool.lock);
	lock_init(&user_pool.lock);
	
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = P2V((void*)(page_table_size + end_roundup + KVIRTUAL_BITMAP_OFF * PAGE_SIZE));
	kernel_vaddr.vaddr_start = 0xffffffff;//这个目前保留着,这是个位图对高128M的映射地址是不确定的，start的意义不大
	cga_putstr(" kvir_pool_bitmap_start:");put_int2hex((int)kernel_vaddr.vaddr_bitmap.bits);
	cga_putstr(" 	kvir_pool_vir_addr_start:");put_int2hex(kernel_vaddr.vaddr_start);
	cga_putstr(" 	kvir_pool_btmp_bytes_len:");put_int2hex(kernel_vaddr.vaddr_bitmap.btmp_bytes_len);
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	cga_putstr("   mem_pool_init done\n");
}

//初始化各个内存块链表
void block_desc_init(struct mem_block_desc* desc_array) {				   
	uint16_t desc_idx, block_size = 16;
	
	for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
		desc_array[desc_idx].block_size = block_size;
		desc_array[desc_idx].blocks_per_arena = (PAGE_SIZE - sizeof(struct arena)) / block_size;	  
		list_init(&desc_array[desc_idx].free_list);
		block_size *= 2;         
	}
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct virtual_addr *user_vaddr) {
	user_vaddr->vaddr_start = USER_VADDR_START;
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8 , PAGE_SIZE);
	user_vaddr->vaddr_bitmap.bits = kalloc(bitmap_pg_cnt * PAGE_SIZE);
	memset(user_vaddr->vaddr_bitmap.bits, 0, bitmap_pg_cnt * PAGE_SIZE); 
	user_vaddr->vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
	bitmap_init(&user_vaddr->vaddr_bitmap);
}

void _mem_init(){
	uint32_t global_memsize = (*(uint32_t*)(P2V(E820_BASE+4)));
	mem_pool_init(global_memsize);
	block_desc_init(k_block_descs);
	cga_putstr("mem_init done\n");	
}


void page_init(){
	cga_putstr("page init!\n");
	_page_init();
}

void mem_init(){
	cga_putstr("mem init!\n");
	_mem_init();
}