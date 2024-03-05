#include "wait_exit.h"
#include "general.h"
#include "assert.h"
#include "kproc.h"
#include "list.h"
#include "types.h"
#include "kalloc.h"
#include "bitmap.h"
#include "fs.h"
#include "file.h"
#include "pipe.h"

/* 释放用户进程资源 */
static void release_prog_resource(struct proc_struct* proc) {
	uint32_t* pgdir_vaddr = proc->pgdir;
	uint16_t user_pde_nr = 768, pde_idx = 0;
	uint32_t pde = 0;
	uint32_t* v_pde_ptr = NULL;	    
	
	uint16_t user_pte_nr = 1024, pte_idx = 0;
	uint32_t pte = 0;
	uint32_t* v_pte_ptr = NULL;	    
	
	uint32_t* first_pte_vaddr_in_pde = NULL;	
	uint32_t pg_phy_addr = 0;
	
	/* 回收页表中用户空间的页框 */
	while (pde_idx < user_pde_nr) {
		v_pde_ptr = pgdir_vaddr + pde_idx;
		pde = *v_pde_ptr;
		if (pde & 0x00000001) {   
			first_pte_vaddr_in_pde = pte_ptr(pde_idx * 0x400000);	  
			pte_idx = 0;
			while (pte_idx < user_pte_nr) {
				v_pte_ptr = first_pte_vaddr_in_pde + pte_idx;
				pte = *v_pte_ptr;
				if (pte & 0x00000001) {
					
					pg_phy_addr = pte & 0xfffff000;
					mempoll_free(PF_USER, pg_phy_addr, 1);
				}
				pte_idx++;
			}
			
			pg_phy_addr = pde & 0xfffff000;
			mempoll_free(PF_USER, pg_phy_addr, 1);
		}
		pde_idx++;
	}
	
	/* 回收用户虚拟地址池所占的物理内存*/
	uint32_t bitmap_pg_cnt = (proc->user_mem.vaddr_bitmap.btmp_bytes_len) / PAGE_SIZE;
	uint8_t* user_vaddr_pool_bitmap = proc->user_mem.vaddr_bitmap.bits;
	kfree(user_vaddr_pool_bitmap);
	
	/* 关闭进程打开的文件 */
	uint8_t local_fd = 3;
	while(local_fd < MAX_FILES_OPEN_PER_PROC) {
		if (proc->fd_table[local_fd] != -1) {
			if (is_pipe(local_fd)) {
				uint32_t global_fd = fd_local2global(local_fd);  
				if (--file_table[global_fd].fd_pos == 0) {
					free_a_kpage(file_table[global_fd].fd_inode);
					file_table[global_fd].fd_inode = NULL;
				}
			} else {
				sys_close(local_fd);
			}
		}
		local_fd++;
	}
}

/* 查找pelem的parent_pid是否是ppid,成功返回true,失败则返回false */
static bool find_child(struct list_elem* pelem, int32_t ppid) {
   struct proc_struct* pthread = STRUCT_ENTRY(struct proc_struct, global_list_tag, pelem);
   if (pthread->parent_pid == ppid) {     
      return true;  
   }
   return false;     
}

/* list_traversal的回调函数,
 * 查找状态为TASK_HANGING的任务 */
static bool find_hanging_child(struct list_elem* pelem, int32_t ppid) {
   struct proc_struct* pthread = STRUCT_ENTRY(struct proc_struct, global_list_tag, pelem);
   if (pthread->parent_pid == ppid && pthread->status == HANGING) {
      return true;
   }
   return false; 
}

/* list_traversal的回调函数,
 * 将一个子进程过继给init */
static bool init_adopt_a_child(struct list_elem* pelem, int32_t pid) {
   struct proc_struct* pthread = STRUCT_ENTRY(struct proc_struct, global_list_tag, pelem);
   if (pthread->parent_pid == pid) {     
      pthread->parent_pid = 1;
   }
   return false;		
}

/* 等待子进程调用exit,将子进程的退出状态保存到status指向的变量,成功则返回子进程的pid,失败则返回-1 */
pid_t sys_wait(int32_t* status) {
	struct proc_struct* parent_proc = get_cur_proc();
	while(1) {
		struct list_elem* child_elem = list_traversal(&global_proc_list, find_hanging_child, parent_proc->pid);
		if (child_elem != NULL) {
			struct proc_struct* child_proc = STRUCT_ENTRY(struct proc_struct, global_list_tag, child_elem);
			*status = child_proc->exit_status; 
			uint16_t child_pid = child_proc->pid;
			proc_exit(child_proc, false); 
			printk("child waited! %d\n",child_pid);
			return child_pid;
		} 
	
		child_elem = list_traversal(&global_proc_list, find_child, parent_proc->pid);
		if (child_elem == NULL) {	 
			return -1;
		} else {
			proc_block(WAITING); 
		}
	}
}

/* 子进程用来结束自己时调用 */
void sys_exit(int32_t status) {
	struct proc_struct* child_proc = get_cur_proc();
	child_proc->exit_status = status; 
	if (child_proc->parent_pid == -1) {
		PANIC("sys_exit: child_proc->parent_pid is -1\n");
	}
	
	list_traversal(&global_proc_list, init_adopt_a_child, child_proc->pid);
	
	release_prog_resource(child_proc); 
	
	struct proc_struct* parent_proc = pid2proc(child_proc->parent_pid);
	if (parent_proc->status == WAITING) {
		proc_unblock(parent_proc);
	}
	printk("child exit! %d \n",child_proc->pid);
	
	proc_block(HANGING);
}
