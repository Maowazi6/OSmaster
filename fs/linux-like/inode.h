#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "types.h"
#include "list.h"
#include "ide.h"
#include "super_block.h"
#include "vfs.h"
/* inode结构 */
struct llfs_inode {
   uint32_t i_no;    // inode编号

/* 当此inode是文件时,i_size是指文件大小,
若此inode是目录,i_size是指该目录下所有目录项大小之和*/
   uint32_t i_size;

   uint32_t i_open_cnts;   // 记录此文件被打开的次数
   bool write_deny;	   // 写文件不能并行,进程写文件前检查此标识

/* i_sectors[0-11]是直接块, i_sectors[12]用来存储一级间接块指针 */
   uint32_t i_sectors[13];
   struct list_elem inode_tag;
};

struct llfs_inode_info{
	void *inode_info;//可能是normal file，也可能是dir
	struct inode mm_inode;
};

struct llfs_inode* inode_open(struct partition* part, uint32_t inode_no);
void inode_sync(struct partition* part, struct llfs_inode* inode, void* io_buf);
void inode_init(uint32_t inode_no, struct llfs_inode* new_inode);
void inode_close(struct llfs_inode* inode);
void inode_release(struct partition* part, uint32_t inode_no);
void inode_delete(struct partition* part, uint32_t inode_no, void* io_buf);
#endif
