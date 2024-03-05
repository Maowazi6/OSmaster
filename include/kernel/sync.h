#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "types.h"
#include "list.h"


/* 信号量结构 */
struct semaphore {
   uint8_t  value;
   struct   list waiters;
};

/* 锁结构 */
struct lock {
   struct   list_elem   *holder;	    // 锁的持有者
   struct   semaphore   semaphore;	    // 用二元信号量实现锁
   uint32_t holder_repeat_nr;		    // 锁的持有者重复申请锁的次数
};


void sema_up(struct semaphore* psema);
void sema_down(struct semaphore* psema, struct list_elem *obj);
void sema_init(struct semaphore* psema, uint8_t value);
void lock_init(struct lock* plock);
void sync_acquire(struct lock* plock);
void sync_release(struct lock* plock);
#endif
