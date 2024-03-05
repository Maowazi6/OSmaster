#include "idt.h"
#include "gdt.h"
#include "types.h"
#include "io.h"
#include "cga.h"
#include "kalloc.h"
#define STS_VECTOR 0x80

extern void *idt_vectors[MAX_IDTDESC];
extern uint32_t syscall_handler(void);

struct gate_desc idt[MAX_IDTDESC];
void *idt_function[MAX_IDTDESC];
uint32_t *intr_name[MAX_IDTDESC];		     // 用于保存异常的名字

/* 在中断处理程序数组第vector_no个元素中注册安装中断处理程序function */
void idt_register(uint8_t vector_no, void* function) {
   idt_function[vector_no] = function; 
}

void arch_intr_register(uint8_t vector_no, void* function){
	idt_register(vector_no,function);
}

void arch_disable(){
	cli();
}

void arch_enable(){
	sti();
}

bool get_arch_intr_status(){
	return get_eflag_if();
}


//根据type段的取值来创建4中不同的门描述符
static void create_gate_desc(struct gate_desc *gate_desc, uint32_t type, uint32_t dpl, uint32_t off){
	gate_desc->off_15_0 = (uint32_t)(off) & 0xffff;       
	gate_desc->cs = SEG_KCODE<<3;                            
	gate_desc->keep = 0;                                                            
	gate_desc->type = type; 
	gate_desc->s = 0;                                 
	gate_desc->dpl = dpl;                             
	gate_desc->p = 1;                                 
	gate_desc->off_31_16 = (uint32_t)(off) >> 16;         
}

/*初始化中断描述符表*/
static void idt_desc_init(void) {
	int i;
	for (i = 0; i < MAX_IDTDESC; i++) {
		create_gate_desc(&idt[i], STS_INTGATE, KDPL ,idt_vectors[i]); 
	}
		//系统调用dpl为3
	create_gate_desc(&idt[STS_VECTOR], STS_INTGATE, UDPL, syscall_handler);
	//create_gate_desc(&idt[14], STS_INTGATE, KDPL, 0);
}

/* 默认中断函数 */
static void general_int_handler(uint8_t vec_nr) {
	if (vec_nr == 0x27 || vec_nr == 0x2f) {	
		return;		//IRQ7和IRQ15会产生伪中断,无须处理。
	}
	if (vec_nr == 14) {	  // 若为Pagefault,将缺失的地址打印出来并悬停
		int page_fault_vaddr = 0; 
		asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));	  // cr2是存放造成page_fault的地址
		cga_putstr("\npage fault addr is 0x");put_int2hex(page_fault_vaddr); 
   }
   while(1);
}

/* 完成一般中断处理函数注册及异常名称注册 */
static void exception_init(void) {			
	int i;
	for (i = 0; i < MAX_IDTDESC; i++) {
		idt_function[i] = general_int_handler;		    					   
		intr_name[i] = "unknown";				    
	}
	intr_name[0] = "#DE Divide Error";
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMI Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF Overflow Exception";
	intr_name[5] = "#BR BOUND Range Exceeded Exception";
	intr_name[6] = "#UD Invalid Opcode Exception";
	intr_name[7] = "#NM Device Not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invalid TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	// int_name[15] 第15项是intel保留项，未使用
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";
}

void arch_intr_init(){
	idt_desc_init();
	exception_init();
	lidt(idt,sizeof(idt));
}


void set_trapframe(struct trap_context *sp){
	sp->edi = 0;
	sp->esi = 0;
	sp->ebp = 0;
	sp->esp = 0;
	sp->ebx = 0;
	sp->edx = 0;
	sp->ecx = 0;
	sp->eax = 0;
	
	sp->gs = 0;		 // 用户态用不上,直接初始为0
	sp->ds = ((SEG_UDATA<<3) + (TI_GDT<<2) + UDPL);
	sp->es = ((SEG_UDATA<<3) + (TI_GDT<<2) + UDPL);
	sp->fs = 0;
	
	//sp->eip = filename;	 // 待执行的用户程序地址
	sp->cs = ((SEG_UCODE<<3) + TI_GDT + UDPL);
	sp->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
	//sp->esp = (void*)((uint32_t)sys_malloc(PAGE_SIZE) + PAGE_SIZE) ;上调度的时候申请
	sp->ss = ((SEG_UDATA<<3) + TI_GDT + UDPL); 
}