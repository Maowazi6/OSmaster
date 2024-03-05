#ifndef _MM_H
#define _MM_H

#include "types.h"
#include "sync.h"
#include "bitmap.h"
#include "list.h"
#include "mmlayout.h"

#define E820_BASE 0xfff0
#define MEM_BITMAP_PAGESIZE  21    //用户内存池、虚拟内存池、内核内存池总共21页，一个7页主要是为了方便，这个虚拟内存池实际上只需要128M 一个页就好了
#define PKERNEL_BITMAP_OFF 0
#define PUSER_BITMAP_OFF 7
#define KVIRTUAL_BITMAP_OFF 14
#define DESC_CNT 7


/* 内存池标记,用于判断用哪个内存池 */
enum pool_flags {
   PF_KERNEL = 1,    // 内核内存池
   PF_USER = 2	     // 用户内存池
};

//内存池结构,生成两个实例用于管理内核内存池和用户内存池 
struct pool {
   struct bitmap pool_bitmap;	 // 本内存池用到的位图结构,用于管理物理内存
   uint32_t phy_addr_start;	 // 本内存池所管理物理内存的起始地址
   uint32_t pool_size;		 // 本内存池字节容量
   struct lock lock;		 // 申请内存时互斥
};

struct arena {
   struct mem_block_desc* desc;	 // 此arena关联的mem_block_desc
/* large为ture时,cnt表示的是页框数。
 * 否则cnt表示空闲mem_block数量 */
   uint32_t cnt;
   bool large;		   
};

/* 用于虚拟地址管理 */
struct virtual_addr {
   struct bitmap vaddr_bitmap;
   uint32_t vaddr_start;
};

/* 内存块 */
struct mem_block {
   struct list_elem free_elem;
};

/* 内存块描述符 */
struct mem_block_desc {
   uint32_t block_size;		 // 内存块大小
   uint32_t blocks_per_arena;	 // 本arena中可容纳此mem_block的数量.
   struct list free_list;	 // 目前可用的mem_block链表
};

void mem_init();
void page_init();
#endif