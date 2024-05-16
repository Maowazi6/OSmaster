#include "inode.h"
#include "fs.h"
#include "file.h"
#include "general.h"
#include "assert.h"
#include "kalloc.h"
#include "kproc.h"
#include "interrupt.h"
#include "list.h"
#include "console.h"
#include "string.h"
#include "super_block.h"
#include "blk_dev.h"
#include "vfs.h"

/* 用来存储inode位置 */
struct inode_position {
   bool	 two_sec;		// inode是否跨扇区
   uint32_t sec_lba;	// inode所在的扇区号
   uint32_t off_size;	// inode在扇区内的字节偏移量
};

const struct super_operations llfs_sb_ops = {
	.alloc_inode = llfs_alloc_inode
};

const struct dentry_operations llfs_dentry_ops = {
	
};

/* 获取inode所在的扇区和扇区内的偏移量 */
static void inode_locate(struct partition* part, uint32_t inode_no, struct inode_position* inode_pos) {
   ASSERT(inode_no < 4096);
   uint32_t inode_table_lba = part->sb->inode_table_lba;

   uint32_t inode_size = sizeof(struct llfs_inode);
   uint32_t off_size = inode_no * inode_size;	    
   uint32_t off_sec  = off_size / 512;		    	
   uint32_t off_size_in_sec = off_size % 512;	    

   /* 判断此i结点是否跨越2个扇区 */
   uint32_t left_in_sec = 512 - off_size_in_sec;
   if (left_in_sec < inode_size ) {
      inode_pos->two_sec = true;
   } else {
      inode_pos->two_sec = false;
   }
   inode_pos->sec_lba = inode_table_lba + off_sec;
   inode_pos->off_size = off_size_in_sec;
}

/* 将inode写入到分区part */
void inode_sync(struct partition* part, struct llfs_inode* inode, void* io_buf) {	 // io_buf是用于硬盘io的缓冲区
	uint8_t inode_no = inode->i_no;
	struct inode_position inode_pos;
	inode_locate(part, inode_no, &inode_pos);	       // inode位置信息会存入inode_pos
	ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));
	
	struct llfs_inode pure_inode;
	memcpy(&pure_inode, inode, sizeof(struct llfs_inode));

	pure_inode.i_open_cnts = 0;
	pure_inode.write_deny = false;
	pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;
	
	char* inode_buf = (char*)io_buf;
	if (inode_pos.two_sec) {	    // 跨了两个扇区
		part->blk->blk_ops->blk_read(part->blk,inode_pos.sec_lba,2,inode_buf);
		memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct llfs_inode));
		part->blk->blk_ops->blk_write(part->blk, inode_pos.sec_lba, 2, inode_buf);
	} else {
		part->blk->blk_ops->blk_read(part->blk,inode_pos.sec_lba,1,inode_buf);
		memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct llfs_inode));
		part->blk->blk_ops->blk_write(part->blk, inode_pos.sec_lba, 1, inode_buf);
   }
}

struct llfs_inode* inode_open(struct partition* part, uint32_t inode_no) {
	struct list_elem* elem = part->open_inodes.head.next;
	struct llfs_inode* inode_found;
	while (elem != &part->open_inodes.tail) {
		inode_found = STRUCT_ENTRY(struct llfs_inode, inode_tag, elem);
		if (inode_found->i_no == inode_no) {
			inode_found->i_open_cnts++;
			return inode_found;
		}
		elem = elem->next;
	}
	struct inode_position inode_pos;
	inode_locate(part, inode_no, &inode_pos);
	struct proc_struct* cur = get_cur_proc();
	uint32_t* cur_pagedir_bak = cur->pgdir;
	cur->pgdir = NULL;
	inode_found = (struct llfs_inode*)kalloc(sizeof(struct llfs_inode));
	cur->pgdir = cur_pagedir_bak;
	
	char* inode_buf;
	if (inode_pos.two_sec) {	// 考虑跨扇区的情况
		inode_buf = (char*)kalloc(1024);
	
		part->blk->blk_ops->blk_read(part->blk, inode_pos.sec_lba, 2, inode_buf);
	} else {
		inode_buf = (char*)kalloc(512);
		part->blk->blk_ops->blk_read(part->blk, inode_pos.sec_lba, 1, inode_buf);
	}
	memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct llfs_inode));
	list_push(&part->open_inodes, &inode_found->inode_tag);
	inode_found->i_open_cnts = 1;
	kfree(inode_buf);
	return inode_found;
}

/* 关闭inode或减少inode的打开数 */
void inode_close(struct llfs_inode* inode) {
	bool old_status = set_intr(false);
	if (--inode->i_open_cnts == 0) {
		list_remove(&inode->inode_tag);
	
		struct proc_struct* cur = get_cur_proc();
		uint32_t* cur_pagedir_bak = cur->pgdir;
		cur->pgdir = NULL;
		kfree(inode);		         // 释放inode的内核空间
		//inode = NULL;
		cur->pgdir = cur_pagedir_bak;
	}
	set_intr(old_status);
}

/* 将硬盘分区part上的inode清空 */
void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf) {
	ASSERT(inode_no < 4096);
	struct inode_position inode_pos;
	inode_locate(part, inode_no, &inode_pos);     // inode位置信息会存入inode_pos
	ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));
	
	char* inode_buf = (char*)io_buf;
	if (inode_pos.two_sec) {   // inode跨扇区,读入2个扇区
		part->blk->blk_ops->blk_read(part->blk, inode_pos.sec_lba, 2, inode_buf);
		memset((inode_buf + inode_pos.off_size), 0, sizeof(struct llfs_inode));
		part->blk->blk_ops->blk_write(part->blk, inode_pos.sec_lba, 2, inode_buf);
	} else {
		part->blk->blk_ops->blk_read(part->blk, inode_pos.sec_lba, 1, inode_buf);
		memset((inode_buf + inode_pos.off_size), 0, sizeof(struct llfs_inode));
		part->blk->blk_ops->blk_write(part->blk, inode_pos.sec_lba, 1, inode_buf);
	}
}

/* 回收inode的数据块和inode本身 */
void inode_release(struct partition* part, uint32_t inode_no) {
	struct llfs_inode* inode_to_del = inode_open(part, inode_no);
	ASSERT(inode_to_del->i_no == inode_no);
	
	uint8_t block_idx = 0, block_cnt = 12;
	uint32_t block_bitmap_idx;
	uint32_t all_blocks[140] = {0};	  
	
	/*先将前12个直接块存入all_blocks*/
	while (block_idx < 12) {
		all_blocks[block_idx] = inode_to_del->i_sectors[block_idx];
		block_idx++;
	}
	if (inode_to_del->i_sectors[12] != 0) {
		part->blk->blk_ops->blk_read(part->blk, inode_to_del->i_sectors[12], 1, all_blocks + 12);
		block_cnt = 140;
		block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
		ASSERT(block_bitmap_idx > 0);
		bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
		bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);
	}
	block_idx = 0;
	while (block_idx < block_cnt) {
		if (all_blocks[block_idx] != 0) {
		block_bitmap_idx = 0;
		block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;
		ASSERT(block_bitmap_idx > 0);
		bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
		bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);
		}
		block_idx++; 
	}
	bitmap_set(&part->inode_bitmap, inode_no, 0);  
	bitmap_sync(part, inode_no, INODE_BITMAP);
	
	void* io_buf = kalloc(1024);
	inode_delete(part, inode_no, io_buf);
	kfree(io_buf);
	inode_close(inode_to_del);
}

/* 初始化new_inode */
void inode_init(uint32_t inode_no, struct llfs_inode* new_inode) {
	new_inode->i_no = inode_no;
	new_inode->i_size = 0;
	new_inode->i_open_cnts = 0;
	new_inode->write_deny = false;
	
	/* 初始化块索引数组i_sector */
	uint8_t sec_idx = 0;
	while (sec_idx < 13) {
	/* i_sectors[12]为一级间接块地址 */
		new_inode->i_sectors[sec_idx] = 0;
		sec_idx++;
	}
}
