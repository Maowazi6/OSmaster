#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H
#include "types.h"
#include "bitmap.h"
#include "list.h"


/* 超级块 */
struct llfs_super_block {
   uint32_t magic;		    // 用来标识文件系统类型,支持多文件系统的操作系统通过此标志来识别文件系统类型
   uint32_t sec_cnt;		    // 本分区总共的扇区数
   uint32_t inode_cnt;		    // 本分区中inode数量
   uint32_t part_lba_base;	    // 本分区的起始lba地址

   uint32_t block_bitmap_lba;	    // 块位图本身起始扇区地址
   uint32_t block_bitmap_sects;     // 扇区位图本身占用的扇区数量

   uint32_t inode_bitmap_lba;	    // i结点位图起始扇区lba地址
   uint32_t inode_bitmap_sects;	    // i结点位图占用的扇区数量

   uint32_t inode_table_lba;	    // i结点表起始扇区lba地址
   uint32_t inode_table_sects;	    // i结点表占用的扇区数量

   uint32_t data_start_lba;	    // 数据区开始的第一个扇区号
   uint32_t root_inode_no;	    // 根目录所在的I结点号
   uint32_t dir_entry_size;	    // 目录项大小

   uint8_t  pad[460];		    // 加上460字节,凑够512字节1扇区大小
} __attribute__ ((packed));

/* 分区结构 */
struct partition {
   uint32_t start_lba;		 // 起始扇区
   uint32_t sec_cnt;		 // 扇区数
   struct disk* my_disk;	 // 分区所属的硬盘
   struct blk_dev* blk;	 // 分区所属的硬盘
   char *name;		 // 分区名称
   struct list_elem part_tag;	 // 用于队列中的标记
   struct llfs_super_block* sb;	 // 本分区的超级块
   struct bitmap block_bitmap;	 // 块位图
   struct bitmap inode_bitmap;	 // i结点位图
   struct list open_inodes;	 // 本分区打开的i结点队列
};
#endif
