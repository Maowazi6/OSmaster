#include "gdt.h"
#include "types.h"
#include "tss.h"


struct gdt_desc gdt_table[NSEGS];   

//4kb粒度lim表示高20位  一个段最大4GB
void create_normal_desc(struct gdt_desc *normal_desc, uint32_t base, uint32_t lim, int dpl, int type){
	normal_desc->lim_15_0 		= (lim >> 12) & 0xffff;  
	normal_desc->base_15_0 		= base & 0xffff; 
	normal_desc->base_23_16 	= (base >> 16) & 0xff; 
	normal_desc->type 			= type;       
	normal_desc->s 				= 1;          
	normal_desc->dpl 			= dpl;        
	normal_desc->p 				= 1;          
	normal_desc->lim_19_16 		= (lim >> 28) & 0xf;  
	normal_desc->avl 			= 0;        
	normal_desc->rsv1 			= 0;       
	normal_desc->db 			= 1;         
	normal_desc->g 				= 1;          
	normal_desc->base_31_24 	= (base>>24) & 0xff;
}


//1byte粒度lim表示低20位  一个段最大1MB,  TSS用的就是这个，因为他很小
void create_small_desc(struct gdt_desc *normal_desc, uint32_t base, uint32_t lim, int dpl, int type){
	normal_desc->lim_15_0 		= lim & 0xffff;  
	normal_desc->base_15_0 		= base & 0xffff; 
	normal_desc->base_23_16 	= (base >> 16) & 0xff; 
	normal_desc->type 			= type;       
	normal_desc->s 				= 1;          
	normal_desc->dpl 			= dpl;        
	normal_desc->p 				= 1;          
	normal_desc->lim_19_16 		= (lim >> 16) & 0xf;  
	normal_desc->avl 			= 0;        
	normal_desc->rsv1 			= 0;       
	normal_desc->db 			= 1;         
	normal_desc->g 				= 0;          
	normal_desc->base_31_24 	= (base>>24) & 0xff;
}


void gdt_init(){
	tss_init();
	
	create_normal_desc(&gdt_table[SEG_KCODE], 0x0, 0xffffffff, KDPL, STA_R | STA_X);
	create_normal_desc(&gdt_table[SEG_KDATA], 0x0, 0xffffffff, KDPL, STA_W);
	create_normal_desc(&gdt_table[SEG_UCODE], 0x0, 0xffffffff, UDPL, STA_X|STA_R);
	create_normal_desc(&gdt_table[SEG_UDATA], 0x0, 0xffffffff, UDPL, STA_W);
	
	lgdt(gdt_table,sizeof(struct gdt_desc) * NSEGS);
	ltr(SEG_TSS << 3);
}