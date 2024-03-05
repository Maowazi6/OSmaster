#include "string.h"
#include "assert.h"
#include "general.h"
#include "kalloc.h"

extern char end[];  
uint32_t *pg_dir;  //这是个物理地址
extern void *kalloc();

/* 得到虚拟地址vaddr对应的pte指针*/
uint32_t* pte_ptr(uint32_t vaddr){
	//这一步的操作就是 取到pagedir 然后vaddr>>10位实际上就是一个pde的偏移值
   uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
   return pte;
}   
/* 得到虚拟地址映射到的物理地址 */
uint32_t addr_v2p(uint32_t vaddr){
	//0xfffff000实际上就是取页目录的最后一项的页框   然后又偏移到这个页框的最后一项
	uint32_t* pte = pte_ptr(vaddr);
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

/* 得到虚拟地址vaddr对应的pde的指针 */
uint32_t* pde_ptr(uint32_t vaddr){
   uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
   return pde;
}

/* 去掉页表中虚拟地址vaddr的映射,只去掉vaddr对应的pte */
void page_table_pte_remove(uint32_t vaddr){
   uint32_t* pte = pte_ptr(vaddr);
   *pte &= ~PG_P_TRUE;	// 将页表项pte的P位置0
   asm volatile ("invlpg %0"::"m" (vaddr):"memory");    //更新tlb
}



static struct kmap{
  void *virt;
  uint32_t phys_start;
  uint32_t phys_end;
} kmap[] = {
 { (void*)KERN_BASE, 0, KERN_END}, // kernel space
};

//建立线性地址映射，这个只是用户线程的映射高128M的内存也可以使用这个函数，内核直接映射部分不需要
void page_table_add(void* _vaddr, void* _page_phyaddr){
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);  
	uint32_t* pte = pte_ptr(vaddr);
	//cga_putstr("pde\n");put_int2hex(pde);
	//cga_putstr("pde\n");put_int2hex(*pde);
	if(*pde & 0x00000001){
		ASSERT(!(*pte & 0x00000001));
		if(!(*pte & 0x00000001)){  
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_TRUE);    
		}else{	  
			PANIC("pte repeat");
		}
	}else{	
		uint32_t pde_phyaddr = (uint32_t)malloc_a_kpage();
		*pde = (V2P(pde_phyaddr) | PG_US_U | PG_RW_W | PG_P_TRUE);
		memset((void*)((int)pte & 0xfffff000), 0, PAGE_SIZE); 
		ASSERT(!(*pte & 0x00000001));
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_TRUE);      
	}
}


static uint32_t *walkpgdir(uint32_t *pgdir, const void *va, int alloc){
	uint32_t *pde;
	uint32_t *pgtab;
	pde = &pgdir[PDE_IDX(va)];  
	if(*pde & PG_P_TRUE){
		pgtab = (uint32_t*)P2V(PTE_ADDR(*pde));		
	} else {
		PANIC("pde not exist");
	}
	return &pgtab[PTE_IDX(va)];
}


static uint32_t mappages(uint32_t *pgdir, void *va, uint32_t size, uint32_t pa){
	char *a, *last;
	uint32_t *pte;
	a = (char*)PGROUNDDOWN((uint32_t)va);
	last = (char*)PGROUNDDOWN(((uint32_t)va) + size - 1);
	for(;;){
		if((pte = walkpgdir(pgdir, a, 1)) == 0){
			return -1;
		}
		if(*pte & PG_P_TRUE){
			PANIC("remap");
		}
		*pte = pa | PG_US_U | PG_RW_W | PG_P_TRUE;//调试时用这个，毕竟内核态才可以访问内核部分代码
		//*pte = pa | PG_RW_W | PG_P_TRUE;
		if(a == last)
			break;
		a += PAGE_SIZE;
		pa += PAGE_SIZE;
	}
	return 0;
}

static void pg_pte_init(){
	struct kmap *k;
	for(k = kmap; k < &kmap[NELEM(kmap)]; k++){
		if(mappages(pg_dir, k->virt, k->phys_end - k->phys_start,(uint32_t)k->phys_start) < 0) {
			PANIC("mappages false");
		}
	}
}

//建立内核页目录的各个pde
static void pg_dir_init(){
	pg_dir = V2P(PGROUNDUP(end));
	cga_putstr("pg_dir:");
	put_int2hex((uint32_t)pg_dir);
	cga_putstr("\n");
	memset((void*)pg_dir, 0, PAGE_SIZE);
	uint32_t *pde = (uint32_t)pg_dir + KPDE_OFF * 4;
	uint32_t paddr = (uint32_t)pg_dir + PAGE_SIZE;
	for(int i = 0;i < KPDE_NUM;i++){
		memset((void*)paddr, 0, PAGE_SIZE);
		*pde = paddr | PG_US_U | PG_RW_W | PG_P_TRUE;
		//*pde = paddr  | PG_RW_W | PG_P_TRUE;
		pde++;
		paddr += PAGE_SIZE;
	}
	pde = pg_dir;
	pde += 1023;
	*pde = (uint32_t)pg_dir | PG_US_U | PG_RW_W | PG_P_TRUE;
	//pg_dir[0] = (uint32_t)pg_dir + PAGE_SIZE | PG_US_U | PG_RW_W | PG_P_TRUE;
	return;
}


void _page_init(){
	pg_dir_init();
	pg_pte_init();
	lcr3((uint32_t)pg_dir);
	cga_putstr("cr3 is loaded!");
	cga_putstr("\n");
}


void page_dir_activate(uint32_t *proc_pgdir) {
	uint32_t pagedir_phy_addr = pg_dir; //内核线程默认是这个页表
	
	if (proc_pgdir != NULL)	{//用户页表
		pagedir_phy_addr = addr_v2p((uint32_t)proc_pgdir);
	}
	
	asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}



uint32_t* create_page_dir(void) {
	uint32_t* page_dir_vaddr = malloc_a_kpage();
	if (page_dir_vaddr == NULL) {
		console_put_str("create_page_dir: get_kernel_page failed!");
		return NULL;
	}
	
	memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300*4), (uint32_t*)(0xfffff000+0x300*4), 1024);
	
	uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
	
	page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_TRUE;
	
	return page_dir_vaddr;
}

void free_page_dir(uint32_t *page_dir){
	free_a_kpage(page_dir);
}



