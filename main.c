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
void init();
void init_shell();


int main(){
	init();
	uproc_alloc(init_shell, "init");
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
	sti();
	ide_init();
	filesys_init();
}

void write_app(){
	uint32_t file_size = 6704; 
	uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
	struct disk* sda = &channels[0].devices[0];
	void* prog_buf = kalloc(file_size);
	ide_read(sda, 1024, prog_buf, sec_cnt);
	int32_t fd = sys_open("/print", O_CREAT|O_RDWR);
	if (fd != -1) {
		if(sys_write(fd, prog_buf, file_size) == -1) {
			printk("file write error!\n");
			while(1);
		}
	}
	kfree(prog_buf);
}


