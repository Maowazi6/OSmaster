#ifndef __FS_DIR_H
#define __FS_DIR_H
#include "types.h"
#include "inode.h"
#include "fs.h"
#include "ide.h"
#include "general.h"
#include "types.h"
#include "vfs.h"
#define MAX_FILE_NAME_LEN  16	 // 最大文件名长度

/* 目录结构 */
struct llfs_dir {
   struct llfs_inode* inode;   
   uint32_t dir_pos;	  // 记录在目录内的偏移
   uint8_t dir_buf[512];  // 目录的数据缓存
};

/* 目录项结构 */
struct llfs_dir_entry {
   char filename[MAX_FILE_NAME_LEN];  // 普通文件或目录名称
   uint32_t i_no;		      // 普通文件或目录对应的inode编号
   enum file_types f_type;	      // 文件类型
};

extern struct llfs_dir root_dir;             // 根目录
void open_root_dir(struct partition* part);
struct llfs_dir* dir_open(struct partition* part, uint32_t inode_no);
void dir_close(struct llfs_dir* dir);
bool search_dir_entry(struct partition* part, struct llfs_dir* pdir, const char* name, struct llfs_dir_entry * dir_e);
void create_dir_entry(char* filename, uint32_t inode_no, uint8_t file_type, struct llfs_dir_entry * p_de);
bool sync_dir_entry(struct partition* part, struct llfs_dir* parent_dir, struct llfs_dir_entry * p_de, void* io_buf);
bool delete_dir_entry(struct partition* part, struct llfs_dir* pdir, uint32_t inode_no, void* io_buf);
struct llfs_dir_entry * dir_read(struct partition* part,struct llfs_dir* dir);
bool dir_is_empty(struct partition* part,struct llfs_dir* dir);
int32_t dir_remove(struct partition* part,struct llfs_dir* parent_dir, struct llfs_dir* child_dir);
#endif
