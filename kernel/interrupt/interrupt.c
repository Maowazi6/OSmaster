#include "types.h"
#include "idt.h"
#include "cga.h"
#include "pic_8259A.h"
#include "syscall.h"


#define SYSCALL_NR 32

void *syscall_tab[SYSCALL_NR];

/*完成有关中断的所有初始化工作*/
void intr_init() {
	cga_putstr("intr_init start\n");
	arch_intr_init();  
	syscall_init();
	pic_init();		   
	cga_putstr("intr_init done\n");
}

void set_intr_noret (bool status){
	if(status){
		intr_enable();
	}else{
		intr_disable();
	}
}

bool intr_get_status(){
	return get_arch_intr_status();
}

bool set_intr(bool status){
	bool oldstatus = intr_get_status();
	set_intr_noret(status);
	return oldstatus;
}

void intr_disable(){
	arch_disable();
}

void intr_enable(){
	arch_enable();
}

void intr_register(uint8_t vector_no, void* function){
	arch_intr_register(vector_no,function);
}