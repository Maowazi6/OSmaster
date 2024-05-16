#include "mm.h"
#include "interrupt.h"
#include "kproc.h"
#include "idt.h"
#include "gdt.h"
#include "ide.h"
#include "fs.h"
#include "usyscall.h"
#include "cmd_shell.h"
#include "general.h"
#include "vfs.h"
void init();
void init_shell();


int main(){
	init();
	cli();
	struct proc_struct *proc = uproc_alloc(init_shell, "init");
	sti();
	console_clear();
	proc_exit(&kernel_proc,true);
	while(1);
	return 0;
}

void init_shell(){
	uint32_t ret_pid = fork();
	if(ret_pid) {  // 父进程
		int status;
		int child_pid;
		while(1) {
			child_pid = wait(&status);
		}
	} else {	  // 子进程
		while(1) {
			my_shell();
		}
	}
}

void init(){
	gdt_init();
	cga_clear();
	intr_init();
	keyboard_init();
	timer_init();
	console_init();
	page_init();
	mem_init();
	proc_init();
	dev_blk_init();
	sti();
	vfs_init();
	ide_init();
	dev_show();
	//filesys_init();
	fs_registed_show();
	root_mount();
	//while(1);
}

