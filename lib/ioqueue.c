#include "ioqueue.h"
#include "interrupt.h"
#include "general.h"
#include "assert.h"
#include "types.h"
#include "sync.h"

/* 初始化io队列ioq */
void ioqueue_init(struct ioqueue* ioq) {
   lock_init(&ioq->lock);     
   ioq->producer = ioq->consumer = NULL;  
   ioq->head = ioq->tail = 0; 
}

/* 返回pos在缓冲区中的下一个位置值 */
static int32_t next_pos(int32_t pos) {
   return (pos + 1) % bufsize; 
}

/* 判断队列是否已满 */
bool ioq_full(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == false);
   return next_pos(ioq->head) == ioq->tail;
}

/* 判断队列是否已空 */
static bool ioq_empty(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == false);
   return ioq->head == ioq->tail;
}

/* 使当前生产者或消费者在此缓冲区上等待 */
static void ioq_wait(struct proc_struct** waiter) {
   ASSERT(*waiter == NULL && waiter != NULL);
   *waiter = get_cur_proc();
   block();
}

/* 唤醒waiter */
static void ioq_wakeup(struct proc_struct** waiter) {
   ASSERT(*waiter != NULL);
   proc_unblock(*waiter); 
   *waiter = NULL;
}

/* 消费者从ioq队列中获取一个字符 */
char ioq_getchar(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == false);

   while (ioq_empty(ioq)) {
      sync_acquire(&ioq->lock);	 
      ioq_wait(&ioq->consumer);
      sync_release(&ioq->lock);
   }

   char byte = ioq->buf[ioq->tail];	  // 从缓冲区中取出
   ioq->tail = next_pos(ioq->tail);	  // 把读游标移到下一位置

   if (ioq->producer != NULL) {
      ioq_wakeup(&ioq->producer);		  // 唤醒生产者
   }

   return byte; 
}

/* 生产者往ioq队列中写入一个字符byte */
void ioq_putchar(struct ioqueue* ioq, char byte) {
   ASSERT(intr_get_status() == false);

   while (ioq_full(ioq)) {
      sync_acquire(&ioq->lock);
      ioq_wait(&ioq->producer);
      sync_release(&ioq->lock);
   }
   ioq->buf[ioq->head] = byte;      // 把字节放入缓冲区中
   ioq->head = next_pos(ioq->head); // 把写游标移到下一位置

   if (ioq->consumer != NULL) {
      ioq_wakeup(&ioq->consumer);          // 唤醒消费者
   }
}

/* 返回环形缓冲区中的数据长度 */
uint32_t ioq_length(struct ioqueue* ioq) {
   uint32_t len = 0;
   if (ioq->head >= ioq->tail) {
      len = ioq->head - ioq->tail;
   } else {
      len = bufsize - (ioq->tail - ioq->head);     
   }
   return len;
}
