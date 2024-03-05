#include "sync.h"
#include "assert.h"
#include "interrupt.h"
#include "cga.h"
#include "general.h"

//如果想使用这个sync结构的话进程模块必须实现这三个接口
extern void block();
extern void unblock();
extern void *get_cur_proc_tag();//进程标识符，任意数据类型都可以，因为list就是这么实现的


void sema_init(struct semaphore* psema, uint8_t value){
	psema->value = value;       // 为信号量赋初值
	list_init(&psema->waiters); //初始化信号量的等待队列
}


void sema_down(struct semaphore* psema, struct list_elem *obj){
/* 关中断来保证原子操作 */
	bool old_status = set_intr(false);
	while(psema->value == 0) {	// 若value为0,表示已经被别人持有
		ASSERT(!elem_find(&psema->waiters, obj));
		if (elem_find(&psema->waiters, (struct list_elem *)obj)) {
			PANIC("sema_down: proc blocked has been in waiters_list\n");
		}
		list_append(&psema->waiters, (struct list_elem *)obj); 
		block();
	}
	psema->value--;
	ASSERT(psema->value == 0);	    
	set_intr(old_status);
}

void sema_up(struct semaphore* psema) {
	bool old_status = set_intr(false);
	ASSERT(psema->value == 0);	    
	if (!list_empty(&psema->waiters)){
		unblock(list_pop(&psema->waiters));
	}
	psema->value++;
	ASSERT(psema->value == 1);	    
	set_intr(old_status);
}


void lock_init(struct lock* plock){
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_init(&plock->semaphore, 1);  // 信号量初值为1
}

static void lock_acquire(struct lock* plock, struct list_elem *obj){
	if (plock->holder != obj) { 
		sema_down(&plock->semaphore, obj);    
		plock->holder = obj;
		ASSERT(plock->holder_repeat_nr == 0);
		plock->holder_repeat_nr = 1;
	} else {
		plock->holder_repeat_nr++;
	}
}

static void lock_release(struct lock* plock, struct list_elem *obj){
	ASSERT(plock->holder == obj);
	if (plock->holder_repeat_nr > 1) {
		plock->holder_repeat_nr--;
		return;
	}
	ASSERT(plock->holder_repeat_nr == 1);
	
	plock->holder = NULL;	   // 把锁的持有者置空放在V操作之前
	plock->holder_repeat_nr = 0;
	sema_up(&plock->semaphore);	   // 信号量的V操作,也是原子操作
}


void sync_acquire(struct lock* plock){
	//bool old_status = set_intr(false);
	lock_acquire(plock,(struct list_elem *)get_cur_proc_tag());
	//set_intr(old_status);
}

void sync_release(struct lock* plock){
	//bool old_status = set_intr(false);
	lock_release(plock,(struct list_elem *)get_cur_proc_tag() );
	//set_intr(old_status);
}

void usema_up(struct semaphore* psema){
	sema_up(psema);
}

void usema_down(struct semaphore* psema){
	sema_down(psema,(struct list_elem *)get_cur_proc_tag() );
}
