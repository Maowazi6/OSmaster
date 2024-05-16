#include "kproc.h"
#include "bitmap.h"
#include "list.h"
#include "sync.h"
#include "types.h"
#include "string.h"
#include "kalloc.h"
#include "assert.h"
#include "console.h"
#include "cga.h"
#include "general.h"
#include "idt.h"
#include "tss.h"
#include "file.h"
#include "vfs.h"


extern uint32_t ticks; 
extern void int_exit(void);
/* pid的位图,最大支持1024个pid */
uint8_t pid_bitmap_bits[128] = {0};
struct pid_pool global_pid_pool;

struct proc_struct kernel_proc;    	// 内核PCB
struct proc_struct *cur_proc;    	// 当前PCB
struct list_elem* cur_proc_tag;		// 当前pcb的对列节点
struct proc_struct *idle_proc;    	// 空闲时休眠进程

struct list ready_proc_list;	    // 就绪队列
struct list global_proc_list;	    // 全局队列
//任务应该从高优先级往低优先级执行
//struct list normal_ready_list;		// 普通就绪队列
//struct list	important_ready_list;	// 紧急就绪队列
//struct list	trivial_ready_list;		// 休闲就绪队列

extern void swtch(struct proc_context** cur, struct proc_context** next);
extern void init(void);

/* 系统空闲时运行的线程 */
static void idle(void* arg UNUSED) {
	while(1) {
		block();     
		//执行hlt时必须要保证目前处在开中断的情况下
		asm volatile ("sti; hlt" : : : "memory");
	}
}

static void idle_create(){
	idle_proc = proc_alloc("idle", 31, idle);
	proc_start(idle_proc);
}

void *get_cur_proc_tag(){
	return (void *)&(cur_proc->ready_list_tag);
}

struct proc_struct* get_cur_proc() {
	return cur_proc;
}

static void pid_pool_init(void) { 
	global_pid_pool.pid_start = 1;
	global_pid_pool.pid_bitmap.bits = pid_bitmap_bits;
	global_pid_pool.pid_bitmap.btmp_bytes_len = 128;
	bitmap_init(&global_pid_pool.pid_bitmap);
	lock_init(&global_pid_pool.pid_lock);
}

static pid_t pid_alloc(void) {
	sync_acquire(&global_pid_pool.pid_lock);
	int32_t bit_idx = bitmap_scan(&global_pid_pool.pid_bitmap, 1);
	bitmap_set(&global_pid_pool.pid_bitmap, bit_idx, 1);
	sync_release(&global_pid_pool.pid_lock);
	return (bit_idx + global_pid_pool.pid_start);
}

void release_pid(pid_t pid) {
	sync_acquire(&global_pid_pool.pid_lock);
	int32_t bit_idx = pid - global_pid_pool.pid_start;
	bitmap_set(&global_pid_pool.pid_bitmap, bit_idx, 0);
	sync_release(&global_pid_pool.pid_lock);
}


pid_t fork_pid(void) {
	return pid_alloc();
}

//这个函数时在进程被创建时由context的eip设置的初始化函数，
//这个函数执行完了之后执行ret指令然后就到了栈上设置的返回到trapret，trapret里面设置的是用户程序
static void fork_ret(){
	//printk("fork_ret!\n");
	struct proc_struct* proc = get_cur_proc();
	proc->ktrap->esp0 = (uint32_t)(uint32_t)(ualloc_stack(USER_VADDR_TOP, &proc->user_mem, 1));
	set_trapframe(proc->ktrap);
}

//初始化context，从context返回后到func开始执行
//内核的kernel_init是不需要的，context页没必要设置，在swtch的时候会自动设置这个kernel_main的context的指针的
void pstack_alloc(struct proc_struct* proc, void* func){
	//初次调用预留好栈空间
	proc->kstack = kalloc(PAGE_SIZE);

	proc->kstack += PAGE_SIZE;
	uint8_t *sp = proc->kstack;
	
	sp -= sizeof(struct trap_context);
	proc->ktrap = (struct trap_context *)sp;
	
	sp -= 4;
	*(uint32_t *)sp = (int32_t)int_exit;
	
	sp -= sizeof(struct proc_context);
	proc->context = (struct proc_context *)sp;
	
	if(func != NULL){
		proc->context->eip = func;  //这个地方不能返回,得死循环,用来创建内核执行流
	}else{
		proc->context->eip = fork_ret;
	}
	
	proc->context->ebp = 0;
	proc->context->ebx = 0;
	proc->context->esi = 0;
	proc->context->edi = 0;
}

/* 初始化线程基本信息 */
void init_proc_basic(struct proc_struct* proc, char* name, enum proc_prio prio) {
	proc->pid = pid_alloc();
	strcpy(proc->proc_name, name);
	
	if (proc == &kernel_proc) {
		proc->status = RUNNING;//只会在第一次初始化时执行一次
	} else {
		proc->status = RUNNABLE;
	}
	
	proc->priority = prio;
	proc->ticks = prio;
	proc->elapsed_ticks = 0;
	proc->pgdir = NULL;
	/* 标准输入输出先空出来 */
	proc->fd_table[0] = 0;
	proc->fd_table[1] = 1;
	proc->fd_table[2] = 2;
	/* 其余的全置为-1 */
	uint8_t fd_idx = 3;
	while (fd_idx < MAX_FILES_OPEN_PER_PROC) {
		proc->fd_table[fd_idx] = -1;
		fd_idx++;
	}
	proc->cwd_inode_nr = 0;	    // 以根目录做为默认工作路径
	proc->parent_pid = -1;        // -1表示没有父进程
	proc->stack_magic = 0x19870916;	  // 自定义的魔数
	
	fs_struct_init(&proc->fs);
	files_struct_init(&proc->files);
}

struct proc_struct* proc_alloc(char *name, enum proc_prio prio, void *func){
	struct proc_struct *proc = kalloc(sizeof(struct proc_struct));
	memset(proc, 0, sizeof(struct proc_struct));
	pstack_alloc(proc, func);
	init_proc_basic(proc, name, prio);
	
	return proc;
}


//将创建的进程加入到就绪队列，等到计时器中断，他就开始被调度器调度执行
void proc_start(struct proc_struct *proc){
	ASSERT(!elem_find(&ready_proc_list, &proc->ready_list_tag));
	list_append(&ready_proc_list, &proc->ready_list_tag);
	
	ASSERT(!elem_find(&global_proc_list, &proc->global_list_tag));
	list_append(&global_proc_list, &proc->global_list_tag);
}


static void proc_activate(struct proc_struct *proc){
	page_dir_activate(proc->pgdir);
	if(proc->pgdir!=NULL){
		update_tss_esp(proc->kstack);
	}
}

//象征性的设置一下内核初始化进程，如果想让栈更客观，新创建一个内核栈，并且得把初始化之前的栈拷贝过来
//这里以后得写一个拷贝栈的函数
static void make_kernel_proc(void) {
	cur_proc = &kernel_proc;
	cur_proc_tag = &kernel_proc.global_list_tag;
	init_proc_basic(&kernel_proc, "kernel_init", 31);
	ASSERT(!elem_find(&global_proc_list, &kernel_proc.global_list_tag));
	list_append(&global_proc_list, &kernel_proc.global_list_tag);
	fs_struct_init(&kernel_proc.fs);
	files_struct_init(&kernel_proc.files);
}

/* 实现任务调度 */
void schedule() {
	ASSERT(intr_get_status() == false);
	struct proc_struct* cur = get_cur_proc(); 
	if (cur->status == RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
		ASSERT(!elem_find(&ready_proc_list, &cur->ready_list_tag));
		list_append(&ready_proc_list, &cur->ready_list_tag);
		cur->ticks = cur->priority;     
		cur->status = RUNNABLE;
	}
	
	/* 如果就绪队列中没有可运行的任务,就唤醒idle */
	if (list_empty(&ready_proc_list)) {
		//console_put_str("idle_proc ");
		proc_unblock(idle_proc);
	}
	
	ASSERT(!list_empty(&ready_proc_list));
	cur_proc_tag = NULL;	  

	cur_proc_tag = list_pop(&ready_proc_list);   
	struct proc_struct* next = STRUCT_ENTRY(struct proc_struct, ready_list_tag, cur_proc_tag);
	next->status = RUNNING;
	cur_proc = next;
	
	proc_activate(next);
	swtch(&cur->context, &next->context);
}

/* 当前线程将自己阻塞,标志其状态为stat. */
void proc_block(enum proc_status stat) {
	ASSERT(((stat == BLOCKED) || (stat == SLEEPING) || (stat == HANGING) || (stat == WAITING)));
	bool old_status = set_intr(false);
	struct proc_struct* proc = get_cur_proc();
	proc->status = stat; 	
	schedule();		      		
	set_intr(old_status);
}

void block(){
	proc_block(BLOCKED);
}

void proc_unblock(struct proc_struct* proc) {
	bool old_status = set_intr(false);
	ASSERT(((proc->status == BLOCKED) || (proc->status == SLEEPING) || (proc->status == HANGING) || (proc->status == WAITING)));
	if (proc->status != RUNNABLE) {
		ASSERT(!elem_find(&ready_proc_list, &proc->ready_list_tag));
		if (elem_find(&ready_proc_list, &proc->ready_list_tag)) {
			PANIC("unblock: blocked thread in ready_list\n");
		}
		list_push(&ready_proc_list, &proc->ready_list_tag);    // 放到队列的最前面,使其尽快得到调度
		proc->status = RUNNABLE;
	} 
	set_intr(old_status);
}

void unblock(struct list_elem *waiter){
	struct proc_struct *wp = STRUCT_ENTRY(struct proc_struct, ready_list_tag, waiter);
	proc_unblock(wp);
}

/* 主动让出cpu,换其它线程运行 */
void proc_yield(void) {
	struct proc_struct* cur = get_cur_proc();   
	bool old_status = set_intr(false);
	ASSERT(!elem_find(&ready_proc_list, &cur->ready_list_tag));
	list_append(&ready_proc_list, &cur->ready_list_tag);
	cur->status = RUNNABLE;
	schedule();
	set_intr(old_status);
}

/* 回收pcb和页表,并将其从调度队列中去除 */
void proc_exit(struct proc_struct* proc, bool need_schedule){
	/* 要保证schedule在关中断情况下调用 */
	set_intr(false);
	proc->status = DIED;
	
	/* 如果不是当前线程,就有可能还在就绪队列中,将其从中删除 */
	if (elem_find(&ready_proc_list, &proc->ready_list_tag)) {
		list_remove(&proc->ready_list_tag);
	}
	if (proc->pgdir) {     // 如是进程,回收进程的页表
		free_page_dir(proc->pgdir);
	}
	
	list_remove(&proc->global_list_tag);
	
	/* 回收pcb所在的页,主线程的pcb不在堆中,跨过 */
	if (proc != &kernel_proc) {
		kfree(proc);
	}
	
	release_pid(proc->pid);
	
	/* 如果需要下一轮调度则主动调用schedule */
	if (need_schedule) {
		schedule();
		PANIC("proc_exit: should not be here\n");
	}
}


/* 比对任务的pid */
static bool pid_check(struct list_elem* pelem, int32_t pid) {
	struct proc_struct* proc = STRUCT_ENTRY(struct proc_struct, global_list_tag, pelem);
	if (proc->pid == pid) {
		return true;
	}
	return false;
}

//根据pid找pcb,若找到则返回该pcb,否则返回NULL 
struct proc_struct* pid2proc(int32_t pid) {
	struct list_elem* pelem = list_traversal(&global_proc_list, pid_check, pid);
	if (pelem == NULL) {
		return NULL;
	}
	struct proc_struct* proc = STRUCT_ENTRY(struct proc_struct, global_list_tag, pelem);
	return proc;
}

/* 时钟的中断处理函数 */
void timer_intr(void) {
	//cga_putstr("schedual ");
	struct proc_struct* proc = get_cur_proc();
	
	ASSERT(proc->stack_magic == 0x19870916);         // 检查栈是否溢出
	
	proc->elapsed_ticks++;	 
	ticks++;	  //从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数
	if (proc->ticks == 0) {	  
		schedule(); 
	} else {				  
		proc->ticks--;
	}
}

//以tick为单位的sleep,任何时间形式的sleep会转换此ticks形式
void ticks_to_sleep(uint32_t sleep_ticks) {
   uint32_t start_tick = ticks;
   while (ticks - start_tick < sleep_ticks) {	   // 若间隔的ticks数不够便让出cpu
      proc_yield();
   }
}

/* 创建用户进程 */
struct proc_struct* uproc_alloc(void* filename, char* name) { 
	struct proc_struct *proc = proc_alloc(name, 31, NULL);
	create_user_vaddr_bitmap(&proc->user_mem);
	proc->pgdir = create_page_dir();
	block_desc_init(proc->u_block_desc);
	proc->ktrap->eip = filename;
	proc_set_root(proc->fs);
	proc_start(proc);
	return proc;
}

/* 以填充空格的方式输出buf */
static void pad_print(char* buf, int32_t buf_len, void* ptr, char format) {
	memset(buf, 0, buf_len);
	uint8_t out_pad_0idx = 0;
	switch(format) {
		case 's':
		out_pad_0idx = sprintf(buf, "%s", ptr);
		break;
		case 'd':
		out_pad_0idx = sprintf(buf, "%d", *((int16_t*)ptr));
		break;
		case 'x':
		out_pad_0idx = sprintf(buf, "%x", *((uint32_t*)ptr));
	}
	
	while(out_pad_0idx < buf_len) { // 以空格填充
		buf[out_pad_0idx] = ' ';
		out_pad_0idx++;
	}
	sys_write(stdout_no, buf, buf_len - 1);
}

/* 用于在list_traversal函数中的回调函数,用于针对线程队列的处理 */
static bool elem2proc_info(struct list_elem* pelem, int arg UNUSED) {
	struct proc_struct* proc = STRUCT_ENTRY(struct proc_struct, global_list_tag, pelem);
	char out_pad[16] = {0};
	
	pad_print(out_pad, 16, &proc->pid, 'd');
	if (proc->parent_pid == -1) {
		pad_print(out_pad, 16, "NULL", 's');
	} else { 
		pad_print(out_pad, 16, &proc->parent_pid, 'd');
	}

	switch (proc->status) {
		case 0:
		pad_print(out_pad, 16, "RUNNING", 's');
		break;
		case 1:
		pad_print(out_pad, 16, "RUNNABLE", 's');
		break;
		case 2:
		pad_print(out_pad, 16, "BLOCKED", 's');
		break;
		case 3:
		pad_print(out_pad, 16, "SLEEPING", 's');
		break;
		case 4:
		pad_print(out_pad, 16, "HANGING", 's');
		break;
		case 5:
		pad_print(out_pad, 16, "DIED", 's');
		break;
		case 6:
		pad_print(out_pad, 16, "ZOMBIE", 's');
		break;
		case 7:
		pad_print(out_pad, 16, "WAITING", 's');
	}
	pad_print(out_pad, 16, &proc->elapsed_ticks, 'x');
	
	memset(out_pad, 0, 16);
	ASSERT(strlen(proc->proc_name) < 17);
	memcpy(out_pad, proc->proc_name, strlen(proc->proc_name));
	strcat(out_pad, "\n");
	sys_write(stdout_no, out_pad, strlen(out_pad));
	return false;	// 此处返回false是为了迎合主调函数list_traversal,只有回调函数返回false时才会继续调用此函数
}

 /* 打印任务列表 */
void sys_ps(void) {
	char* ps_title = "PID            PPID           STAT           TICKS          COMMAND\n";
	sys_write(stdout_no, ps_title, strlen(ps_title));
	list_traversal(&global_proc_list, elem2proc_info, 0);
}



//参数应该由内核调度器部分提供
void* sys_malloc(uint32_t size){
	struct proc_struct *cur = get_cur_proc();
	void *addr = ualloc(size, cur->u_block_desc, &cur->user_mem);
	return addr;
}

void sys_free(uint32_t *addr){
	struct proc_struct *cur = get_cur_proc();
	ufree(addr,&cur->user_mem);
}

pid_t sys_getpid(){
	return get_cur_proc()->pid;
}

/* 初始化线程环境 */
void proc_init(void) {
	cga_putstr("proc_init start\n");
	
	list_init(&ready_proc_list);
	list_init(&global_proc_list);
	pid_pool_init();
	/* 将当前main函数创建为进程 */
	make_kernel_proc();
	
	/* 创建idle线程 */
	idle_create();
	
	cga_putstr("proc_init done\n");
}


