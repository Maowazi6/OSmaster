#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "types.h"
#include "sync.h"
#include "list.h"
#include "bitmap.h"

/* 构建一个16字节大小的结构体,用来存分区表项 */
struct partition_table_entry {
   uint8_t  bootable;		 // 是否可引导	
   uint8_t  start_head;		 // 起始磁头号
   uint8_t  start_sec;		 // 起始扇区号
   uint8_t  start_chs;		 // 起始柱面号
   uint8_t  fs_type;		 // 分区类型
   uint8_t  end_head;		 // 结束磁头号
   uint8_t  end_sec;		 // 结束扇区号
   uint8_t  end_chs;		 // 结束柱面号
   uint32_t start_lba;		 // 本分区起始扇区的lba地址
   uint32_t sec_cnt;		 // 本分区的扇区数目
} __attribute__ ((packed));	 // 保证此结构是16字节大小

/* 引导扇区,mbr或ebr所在的扇区 */
struct boot_sector {
   uint8_t  other[446];		 // 引导代码
   struct   partition_table_entry partition_table[4];       // 分区表中有4项,共64字节
   uint16_t signature;		 // 启动扇区的结束标志是0x55,0xaa,
} __attribute__ ((packed));

/* 分区结构 */
struct partition_info {
   struct disk* my_disk;	 		// 分区所属的硬盘
   struct list_elem part_tag;	 	// 用于队列中的标记
   char name[20];		 			// 分区名称
   struct partition_table_entry p_entry;
};

/* 硬盘结构 */
struct disk {
   char name[8];			   // 本硬盘的名称，如sda等
   struct ide_channel* my_channel;	   // 此块硬盘归属于哪个ide通道
   uint8_t dev_no;			   // 本硬盘是主0还是从1
};

/* ata通道结构 */
struct ide_channel {
   char name[8];		 // 本ata通道名称, 如ata0,也被叫做ide0. 可以参考bochs配置文件中关于硬盘的配置。
   uint16_t port_base;		 // 本通道的起始端口号
   uint8_t irq_no;		 // 本通道所用的中断号
   struct lock lock;
   bool expecting_intr;		 // 向硬盘发完命令后等待来自硬盘的中断
   struct semaphore disk_done;	 // 硬盘处理完成.线程用这个信号量来阻塞自己，由硬盘完成后产生的中断将线程唤醒
   struct disk devices[2];	 // 一个通道上连接两个硬盘，一主一从
};

struct dev_ideblk{
	char *name;
	struct partition_info *ideblk;
	struct list_elem dev_list;
};

void intr_hd_handler(uint8_t irq_no);
void ide_init(void);
extern uint8_t channel_cnt;
extern struct ide_channel channels[];
extern struct list partition_list;
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
#endif
