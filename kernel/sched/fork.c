#include "types.h"
#include "kproc.h"
#include "kalloc.h"
#include "bitmap.h"
#include "list.h"
#include "inode.h"
#include "file.h"
#include "pipe.h"
#include "assert.h"
extern void int_exit(void);


static void proc_fd_copy(struct proc_struct* child_proc, struct proc_struct* parent_proc){
	for(int i = 0;i < MAX_FILES_OPEN_PER_PROC;i++){
		child_proc->fd_table[i] = parent_proc->fd_table[i];
	}
}

/* 将父进程的pcb、虚拟地址位图拷贝给子进程 */
//目前栈限制在4kb
static int32_t copy_pcb(struct proc_struct* child_proc, struct proc_struct* parent_proc) {
	child_proc->kstack = kalloc(PAGE_SIZE);
	memcpy(child_proc->kstack, (uint32_t)parent_proc->kstack - PAGE_SIZE, PAGE_SIZE);
	child_proc->priority = 31;
	child_proc->kstack += PAGE_SIZE;
	child_proc->stack_magic = 0x19870916;
	child_proc->pid = fork_pid();
	child_proc->elapsed_ticks = 0;
	child_proc->status = RUNNABLE;
	child_proc->ticks = child_proc->priority;   
	child_proc->parent_pid = parent_proc->pid;
	child_proc->ready_list_tag.prev = child_proc->ready_list_tag.next = NULL;
	child_proc->global_list_tag.prev = child_proc->global_list_tag.next = NULL;
	block_desc_init(child_proc->u_block_desc);
	
	create_user_vaddr_bitmap(&child_proc->user_mem);
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8 , PAGE_SIZE);
	memcpy(child_proc->user_mem.vaddr_bitmap.bits, parent_proc->user_mem.vaddr_bitmap.bits, bitmap_pg_cnt * PAGE_SIZE);
	
	proc_fd_copy(child_proc, parent_proc);
	
	return 0;
}

/* 复制子进程的进程体(代码和数据)及用户栈 */
static void copy_pagedir(struct proc_struct* child_proc, struct proc_struct* parent_proc, void* buf_page) {
	uint8_t* vaddr_btmp = parent_proc->user_mem.vaddr_bitmap.bits;
	uint32_t btmp_bytes_len = parent_proc->user_mem.vaddr_bitmap.btmp_bytes_len;
	uint32_t vaddr_start = parent_proc->user_mem.vaddr_start;
	uint32_t idx_byte = 0;
	uint32_t idx_bit = 0;
	uint32_t prog_vaddr = 0;
	/* 在父进程的用户空间中查找已有数据的页 */
	while (idx_byte < btmp_bytes_len) {
		if (vaddr_btmp[idx_byte]) {
			idx_bit = 0;
			while (idx_bit < 8) {
				if ((BITMAP_MASK << idx_bit) & vaddr_btmp[idx_byte]) {
					prog_vaddr = (idx_byte * 8 + idx_bit) * PAGE_SIZE + vaddr_start;
					memcpy(buf_page, (void*)prog_vaddr, PAGE_SIZE);
					page_dir_activate(child_proc->pgdir);
					get_a_page_without_opvaddrbitmap(PF_USER, prog_vaddr);
					memcpy((void*)prog_vaddr, buf_page, PAGE_SIZE);
					page_dir_activate(parent_proc->pgdir);
				}
				idx_bit++;
			}
		}
		idx_byte++;
	}
}

/* 为子进程构建stack和修改返回值 */
static int32_t build_child_stack(struct proc_struct* child_proc) {
	uint8_t* sp = child_proc->kstack;
  
	// 修改子进程的返回值为0 ,返回后判断就知道是子进程而不是父进程
    sp -= sizeof(struct trap_context);
	child_proc->ktrap = (struct trap_context *)sp;
	child_proc->ktrap->eax = 0;
	
	//设置swtch context
	sp -= sizeof(struct proc_context);
	child_proc->context = (struct proc_context *)sp;
	child_proc->context-> ebp = 0;
	child_proc->context-> ebx = 0;
	child_proc->context-> edi = 0;
	child_proc->context-> esi = 0;
	child_proc->context-> eip = (int32_t)int_exit;
	return 0;
}



/* 更新inode打开数 */
static void update_inode_open_cnts(struct proc_struct* proc) {
	int32_t local_fd = 3, global_fd = 0;
	while (local_fd < MAX_FILES_OPEN_PER_PROC) {
		global_fd = proc->fd_table[local_fd];
		ASSERT(global_fd < MAX_FILE_OPEN);
		if (global_fd != -1) {
			if (is_pipe(local_fd)) {
				file_table[global_fd].fd_pos++;
			} else {
				file_table[global_fd].fd_inode->i_open_cnts++;
			}
		}
		local_fd++;
	}
}

/* 拷贝父进程本身所占资源给子进程 */
static int32_t copy_process(struct proc_struct* child_proc, struct proc_struct* parent_proc) {
	if (copy_pcb(child_proc, parent_proc) == -1) {
		return -1;
	}
	child_proc->pgdir = create_page_dir();
	if(child_proc->pgdir == NULL) {
		return -1;
	}

	// 内核缓冲区,作为父进程用户空间的数据复制到子进程用户空间的中转 ,父子进程都可访问
	void* buf_page = malloc_a_kpage();
	if (buf_page == NULL) {
		return -1;
	}

	copy_pagedir(child_proc, parent_proc, buf_page);

	build_child_stack(child_proc);

	update_inode_open_cnts(child_proc);

	free_a_kpage(buf_page);
	return 0;
}


/* fork子进程,内核线程不可直接调用 */
//中断后压栈再进入sys_fork,那么当前线程内核栈的顶部一定是intr_frame,子进程就可以通过这个intr_frame来继续父进程的fork函数的下一条指令
pid_t sys_fork(void) {
	struct proc_struct* parent_proc = get_cur_proc();
	struct proc_struct* child_proc = kalloc(sizeof(struct proc_struct));    
	if (child_proc == NULL) {
		return -1;
	}
	
	ASSERT(false == intr_get_status() && parent_proc->pgdir != NULL);
	
	if (copy_process(child_proc, parent_proc) == -1) {
		return -1;
	}
	
	proc_start(child_proc);

	return child_proc->pid;    // 父进程返回子进程的pid
}