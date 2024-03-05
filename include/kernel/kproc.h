#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "types.h"
#include "sync.h"
#include "mm.h"
#include "bitmap.h"
#include "list.h"
#include "idt.h"

#define PROCNAME_LIMIT_LEN 16
#define MAX_FILES_OPEN_PER_PROC 8


typedef int16_t pid_t;

//进程状态
enum proc_status {
   RUNNING,
   RUNNABLE,
   BLOCKED,
   SLEEPING,
   HANGING,
   DIED,
   ZOMBIE,
   WAITING
};

enum proc_prio{
	NORMAL,
	TRIVIAL,
	IMPORTANT
};

struct pid_pool {
   struct bitmap pid_bitmap;  
   uint32_t pid_start;	      
   struct lock pid_lock;      
};


struct proc_struct{
	struct trap_context     *ktrap;
	uint8_t 				*kstack;
	//uint32_t				kstack_size;
	pid_t		 			pid;
	pid_t 					parent_pid;	
	struct proc_context  	*context;
	int8_t 					status;
	char 					proc_name[PROCNAME_LIMIT_LEN];
	uint32_t 				elapsed_ticks;  //时钟，也就是进程一共运行了多久
	struct list_elem 		ready_list_tag;				    
	struct list_elem 		global_list_tag;
	uint32_t				*pgdir;             
	struct virtual_addr 	user_mem;  
	struct mem_block_desc 	u_block_desc[DESC_CNT];   
	uint32_t 				stack_magic;
	int8_t  				exit_status;         
	enum proc_prio 			priority;		//优先级
	uint8_t 				ticks;	    	//进程每次运行多久	
	uint32_t 				fd_table[MAX_FILES_OPEN_PER_PROC];	// 已打开文件数组
	uint32_t 				cwd_inode_nr;	 // 进程所在的工作目录的inode编号
};

extern struct proc_struct kernel_proc;
extern struct list ready_proc_list;
extern struct list global_proc_list;

struct proc_struct* get_cur_proc();
pid_t fork_pid(void);
void pstack_alloc(struct proc_struct* proc, void* func);
void init_proc_basic(struct proc_struct* proc, char* name, enum proc_prio prio);
void schedule(void);
struct proc_struct* proc_alloc(char *name, enum proc_prio prio, void *func);
void proc_start(struct proc_struct *proc);
void proc_block(enum proc_status stat);
void proc_unblock(struct proc_struct* proc);
pid_t fork_pid(void);
void block();
void unblock();
void proc_yield(void);
void release_pid(pid_t pid);
void proc_exit(struct proc_struct* proc, bool need_schedule);
void timer_intr(void);
void proc_init(void);
void *get_cur_proc_tag();
//void proc_activate(struct proc_struct *proc);
void uproc_alloc(void* filename, char* name);
void ticks_to_sleep(uint32_t sleep_ticks);
void sync_release(struct lock *plock);
void sync_acquire(struct lock *plock);
void* sys_malloc(uint32_t size);
void sys_free(uint32_t *addr);
void sys_ps(void);
pid_t sys_getpid();
#endif
