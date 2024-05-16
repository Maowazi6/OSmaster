#include "fs.h"
#include "super_block.h"
#include "inode.h"
#include "dir.h"
#include "list.h"
#include "file.h"
#include "string.h"
#include "ide.h"
#include "general.h"
#include "assert.h"
#include "kalloc.h"
#include "console.h"
#include "ioqueue.h"
#include "pipe.h"
#include "types.h"
#include "vfs.h"
#include "blk_dev.h"

char *llfs_root_name = "/";
struct partition* cur_part;	 // 默认情况下操作的是哪个分区

static inline struct llfs_inode_info* LLFS_INODE(struct inode *inod){
	return container_of(inod,struct llfs_inode_info,mm_inode);
}

int llfs_get_fd(struct inode *inode){
	int fd = -1;
	struct llfs_inode_info *info = LLFS_INODE(inode);
	fd = (int)(info->inode_info);
	return fd;
}

void filesys_init() {
	uint32_t fd_idx = 0;
	while (fd_idx < MAX_FILE_OPEN) {
		file_table[fd_idx++].fd_inode = NULL;
	}
	printk("filesystem init done......\n");
}

struct inode* llfs_alloc_inode(){
	struct llfs_inode_info *newnode = kalloc(sizeof(struct llfs_inode_info));
	return &newnode->mm_inode;
}

struct dentry* vfs_alloc_dentry(struct super_block *sb, char *name){
	struct dentry *newdentry = kalloc(sizeof(struct dentry));
	newdentry->d_op = NULL;
	newdentry->d_mounted = 0;
	newdentry->d_sb = sb;
	newdentry->d_parent = NULL;
	clearcpy(newdentry->d_name,name,strlen(name));
	return newdentry;
}

struct file* vfs_file_alloc(){
	struct file *file = kalloc(sizeof(struct file));
	file->f_op = NULL;
	file->private_data = NULL;
	file->f_path.dry = NULL;
	file->f_path.mnt = NULL;
	return file;
}

int llfs_dir_alloc(struct super_block *sb, struct dentry *dentry){
	struct inode *newinode = sb->s_op->alloc_inode();
	newinode->i_op = &llfs_dir_inode_operations;
	newinode->i_fop = &llfs_dir_file_operations;
	newinode->i_sb = sb;
	newinode->flag |= INODE_DIR_TYPE;
	dentry->d_inode = newinode;
}

void* llfs_dir_open(struct inode *inod, struct file *i_no){
	if((!inod)){
		printk("llfs_dir_open arguemet fault\n");
		return NULL;
	}
	uint32_t nr = (uint32_t)i_no;
	return (void *)dir_open(inod->i_sb->priv,nr);
}

extern struct file_operations llfs_file_ops;

int llfs_file_alloc(struct super_block *sb, struct dentry *dentry){
	struct inode *newinode = sb->s_op->alloc_inode();
	newinode->i_fop = &llfs_file_ops;
	newinode->i_op = NULL;
	newinode->i_sb = sb;
	newinode->flag |= INODE_FILE_TYPE;
	dentry->d_inode = newinode;
}


int llfs_create(struct inode *inode,struct dentry *dentry){
	struct llfs_dir *dir = LLFS_INODE(inode)->inode_info;
	int fd = -1;
	fd = file_create(dentry->d_sb->priv, dir, dentry->d_name, O_RDWR);
	return fd;
}

int llfs_open(struct inode *inode,struct file *filp){
	uint32_t inode_nr = (uint32_t)filp;
	return file_open(inode->i_sb->priv,inode_nr,O_RDWR);
}

struct dentry *llfs_look_up(struct nameidata *nd){
	if(nd->nd_dentry&&nd->nd_dentry->d_mounted!=0){
		//修改nd挂载点
	}
	struct dentry *tmp_dentry = nd->nd_dentry;
	if(!IS_DIR(tmp_dentry->d_inode->flag)){
		printk("target file is not DIR\n");
		return NULL;
	}
	struct llfs_dir_entry *newdentry = kalloc(sizeof(struct llfs_dir_entry));
	struct llfs_dir *dir = LLFS_INODE(tmp_dentry->d_inode)->inode_info;
	bool res = search_dir_entry(nd->nd_mnt->mnt_sb->priv,dir,nd->savedname,newdentry);
	struct dentry *dtry;
	if(!res){
		printk("no such file\n");
		return NULL;
	}else{
		printk("found file :%s  type:%d\n",newdentry->filename,newdentry->f_type);
		dtry = vfs_alloc_dentry(nd->nd_mnt->mnt_sb, newdentry->filename);
		if(newdentry->f_type == FT_DIRECTORY){
			llfs_dir_alloc(nd->nd_mnt->mnt_sb,dtry);
		}else if(newdentry->f_type == FT_REGULAR){
			llfs_file_alloc(nd->nd_mnt->mnt_sb,dtry);
		}else{
			return NULL;
		}
	}
	struct llfs_inode_info *mm_inode = LLFS_INODE(dtry->d_inode);
	mm_inode->inode_info = dtry->d_inode->i_fop->open(dtry->d_inode,(struct file*)newdentry->i_no);
	dtry->d_parent = nd->nd_dentry;
	nd->d_parent = nd->nd_dentry;
	nd->nd_dentry = dtry;
	return dtry;
}

int llfs_rmdir(struct inode *dir, struct dentry *dentry){
	struct llfs_dir *target = LLFS_INODE(dentry->d_inode)->inode_info;
	struct llfs_dir *parent = LLFS_INODE(dir)->inode_info;
	struct partition *part = dir->i_sb->priv;
	if(!dir_is_empty(part,target)){
		printk("dir %s is not empty, it is not allowed to delete a nonempty directory!\n", dentry->d_name);
		return -1;
	}
	if (dir_remove(part, parent, target)== -1){
		printk("dir_remove failed!\n");
		return -1;
	}
	return 0;
}

void partition_init(struct blk_dev* bd,struct partition *part, struct llfs_super_block *sb_buf,char *name){
	part->start_lba = sb_buf->part_lba_base;
	part->blk = bd;
	part->sec_cnt = sb_buf->sec_cnt;
	part->name = name;
	part->sb = sb_buf;
	
	part->block_bitmap.bits = (uint8_t*)kalloc(sb_buf->block_bitmap_sects * SECTOR_SIZE);
	if (part->block_bitmap.bits == NULL) {
		PANIC("alloc memory failed!");
	}
	part->block_bitmap.btmp_bytes_len = sb_buf->block_bitmap_sects * SECTOR_SIZE;
	bd->blk_ops->blk_read(bd,sb_buf->block_bitmap_lba,sb_buf->block_bitmap_sects,(void*)part->block_bitmap.bits);
  
	part->inode_bitmap.bits = (uint8_t*)kalloc(sb_buf->inode_bitmap_sects * SECTOR_SIZE);
	if (part->inode_bitmap.bits == NULL) {
		PANIC("alloc memory failed!");
	}
	part->inode_bitmap.btmp_bytes_len = sb_buf->inode_bitmap_sects * SECTOR_SIZE;
	bd->blk_ops->blk_read(bd,sb_buf->inode_bitmap_lba,sb_buf->inode_bitmap_sects,(void*)part->inode_bitmap.bits);

	list_init(&part->open_inodes);
}

struct super_block* llfs_get_sb(struct fs_type *ft,struct vfsmount *mnt){
	struct llfs_super_block* sb_buf = (struct llfs_super_block*)kalloc(SECTOR_SIZE);
	if (sb_buf == NULL) {
		PANIC("alloc memory failed!");
	}
	struct blk_dev* bd = blkdev_find(mnt->mnt_devname);
	bd->blk_ops->blk_read(bd,bd->start_block+1,1,(void*)sb_buf);
	printk("start_lba:%x ,magic:%x\n",sb_buf->data_start_lba,sb_buf->magic);
	
	struct partition *part = kalloc(sizeof(struct partition));
	partition_init(bd,part,sb_buf,mnt->mnt_devname);
	
	struct super_block *sb = kalloc(sizeof(struct super_block));
	sb->s_op = &llfs_sb_ops;
	sb->priv = part;
	sb->s_bdev = bd;
	
	struct dentry *mnt_dentry = vfs_alloc_dentry(sb,llfs_root_name);
	mnt_dentry->d_parent = mnt_dentry;
	llfs_dir_alloc(sb,mnt_dentry);

	struct llfs_inode_info *mm_inode = LLFS_INODE(mnt_dentry->d_inode);
	//mm_inode->inode_info = (void *)(mnt_dentry->d_inode->i_fop->open(mnt_dentry->d_sb, mnt_dentry->d_inode, sb_buf->root_inode_no));
	mm_inode->inode_info = (void *)(mnt_dentry->d_inode->i_fop->open(mnt_dentry->d_inode,(struct file*)sb_buf->root_inode_no));
	mnt->mnt_root = mnt_dentry;
	mnt->mnt_sb = sb;
	sb->s_root = mnt_dentry;
	
	return sb;
}

const struct fs_type linux_like_fs = {
	.name 		= "linux_like_fs",
	.type 		= 0x83,
	.get_sb 	= llfs_get_sb,
};

void llfs_init(){
	fs_register(&linux_like_fs);
}

/* 将最上层路径名称解析出来 */
char* path_parse(char* pathname, char* name_store) {
	if (pathname[0] == '/') {
		while(*(++pathname) == '/');
	}

	while (*pathname != '/' && *pathname != 0) {
		*name_store++ = *pathname++;
	}
	
	if (pathname[0] == 0) {
		return NULL;
	}
	return pathname; 
}


/* 将文件描述符转化为文件表的下标 */
uint32_t fd_local2global(uint32_t local_fd) {
	struct proc_struct* cur = get_cur_proc();
	int32_t global_fd = cur->fd_table[local_fd];  
	ASSERT(global_fd >= 0 && global_fd < MAX_FILE_OPEN);
	return (uint32_t)global_fd;
} 

/* 关闭文件描述符fd指向的文件,成功返回0,否则返回-1 */
int32_t sys_close(int32_t fd) {
	int32_t ret = -1;   // 返回值默认为-1,即失败
	if (fd > 2) {
		uint32_t global_fd = fd_local2global(fd);
		if (is_pipe(fd)) {
			/* 如果此管道上的描述符都被关闭,释放管道的环形缓冲区 */
			if (--file_table[global_fd].fd_pos == 0) {
				free_a_kpage(file_table[global_fd].fd_inode);
				file_table[global_fd].fd_inode = NULL;
			}
			ret = 0;
		} else {
			ret = file_close(&file_table[global_fd]);
		}
		get_cur_proc()->fd_table[fd] = -1; // 使该文件描述符位可用
	}
	return ret;
}



/* 重置用于文件读写操作的偏移指针,成功时返回新的偏移量,出错时返回-1 */
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence) {
	if (fd < 0) {
		printk("sys_lseek: fd error\n");
		return -1;
	}
	ASSERT(whence > 0 && whence < 4);
	uint32_t _fd = fd_local2global(fd);
	struct llfs_file* pf = &file_table[_fd];
	int32_t new_pos = 0;   //新的偏移量必须位于文件大小之内
	int32_t file_size = (int32_t)pf->fd_inode->i_size;
	switch (whence) {
		/* SEEK_SET 新的读写位置是相对于文件开头再增加offset个位移量 */
		case SEEK_SET:
		new_pos = offset;
		break;
	
		/* SEEK_CUR 新的读写位置是相对于当前的位置增加offset个位移量 */
		case SEEK_CUR:	// offse可正可负
		new_pos = (int32_t)pf->fd_pos + offset;
		break;
	
		/* SEEK_END 新的读写位置是相对于文件尺寸再增加offset个位移量 */
		case SEEK_END:	   // 此情况下,offset应该为负值
		new_pos = file_size + offset;
	}
	if (new_pos < 0 || new_pos > (file_size - 1)) {	 
		return -1;
	}
	pf->fd_pos = new_pos;
	return pf->fd_pos;
}

/* 删除文件(非目录),成功返回0,失败返回-1 */
int32_t sys_unlink(const char* pathname) {
	ASSERT(strlen(pathname) < MAX_PATH_LEN);
	struct partition *part;
	/* 先检查待删除的文件是否存在 */
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	int inode_no = search_file(part, pathname, &searched_record);
	ASSERT(inode_no != 0);
	if (inode_no == -1) {
		printk("file %s not found!\n", pathname);
		dir_close(searched_record.parent_dir);
		return -1;
	}
	if (searched_record.file_type == FT_DIRECTORY) {
		printk("can`t delete a direcotry with unlink(), use rmdir() to instead\n");
		dir_close(searched_record.parent_dir);
		return -1;
	}
	
	/* 检查是否在已打开文件列表(文件表)中 */
	uint32_t file_idx = 0;
	while (file_idx < MAX_FILE_OPEN) {
		if (file_table[file_idx].fd_inode != NULL && (uint32_t)inode_no == file_table[file_idx].fd_inode->i_no) {
			break;
		}
		file_idx++;
	}
	if (file_idx < MAX_FILE_OPEN) {
		dir_close(searched_record.parent_dir);
		printk("file %s is in use, not allow to delete!\n", pathname);
		return -1;
	}
	ASSERT(file_idx == MAX_FILE_OPEN);
	
	/* 为delete_dir_entry申请缓冲区 */
	void* io_buf = kalloc(SECTOR_SIZE + SECTOR_SIZE);
	if (io_buf == NULL) {
		dir_close(searched_record.parent_dir);
		printk("sys_unlink: malloc for io_buf failed\n");
		return -1;
	}
	
	struct llfs_dir* parent_dir = searched_record.parent_dir; 
	delete_dir_entry(part, parent_dir, inode_no, io_buf);
	inode_release(part, inode_no);
	kfree(io_buf);
	dir_close(searched_record.parent_dir);
	return 0;   // 成功删除文件 
}

/* 成功关闭目录dir返回0,失败返回-1 */
int32_t sys_closedir(struct llfs_dir* dir) {
	int32_t ret = -1;
	if (dir != NULL) {
		dir_close(dir);
		ret = 0;
	}
	return ret;
}

/* 读取目录dir的1个目录项,成功后返回其目录项地址,到目录尾时或出错时返回NULL */
struct llfs_dir_entry * sys_readdir(struct llfs_dir* dir) {
	ASSERT(dir != NULL);
	struct partition *part;
	return dir_read(part, dir);
}

/* 把目录dir的指针dir_pos置0 */
void sys_rewinddir(struct llfs_dir* dir) {
	dir->dir_pos = 0;
}

/* 显示系统支持的内部命令 */
void sys_help(void) {
   printk("\
 buildin commands:\n\
       ls: show directory or file information\n\
       cd: change current work directory\n\
       mkdir: create a directory\n\
       rmdir: remove a empty directory\n\
       rm: remove a regular file\n\
       pwd: show current work directory\n\
       ps: show process information\n\
       clear: clear screen\n\
 shortcut key:\n\
       ctrl+l: clear screen\n\
       ctrl+u: clear input\n\n");
}


/* 创建目录pathname,成功返回0,失败返回-1 */
int llfs_mkdir(struct inode *dir,struct dentry *dentry, int flag) {
	uint8_t rollback_step = 0;	       // 用于操作失败时回滚各资源状态
	void* io_buf = kalloc(SECTOR_SIZE * 2);
	if (io_buf == NULL) {
		printk("sys_mkdir: kalloc for io_buf failed\n");
		return -1;
	}
	struct partition *part = dir->i_sb->priv;
	int inode_no = -1;
	inode_no = inode_bitmap_alloc(part); 
	
	if (inode_no == -1) {
		printk("sys_mkdir: allocate inode failed\n");
		rollback_step = 1;
		goto rollback;
	}
	struct llfs_dir* parent_dir = LLFS_INODE(dir)->inode_info;
	
	struct llfs_inode new_dir_inode;
	inode_init(inode_no, &new_dir_inode);
	
	uint32_t block_bitmap_idx = 0;
	int32_t block_lba = -1;
	block_lba = block_bitmap_alloc(part);
	if (block_lba == -1) {
		printk("sys_mkdir: block_bitmap_alloc for create directory failed\n");
		rollback_step = 2;
		goto rollback;
	}
	new_dir_inode.i_sectors[0] = block_lba;
	
	block_bitmap_idx = block_lba - part->sb->data_start_lba;
	ASSERT(block_bitmap_idx != 0);
	bitmap_sync(part, block_bitmap_idx, BLOCK_BITMAP);
	
	memset(io_buf, 0, SECTOR_SIZE * 2);
	struct llfs_dir_entry * p_de = (struct llfs_dir_entry *)io_buf;
	
	/* 初始化当前目录"." */
	memcpy(p_de->filename, ".", 1);
	p_de->i_no = inode_no ;
	p_de->f_type = FT_DIRECTORY;
	
	p_de++;
	/* 初始化当前目录".." */
	memcpy(p_de->filename, "..", 2);
	p_de->i_no = parent_dir->inode->i_no;
	p_de->f_type = FT_DIRECTORY;
	part->blk->blk_ops->blk_write(part->blk, new_dir_inode.i_sectors[0], 1, io_buf);
	
	new_dir_inode.i_size = 2 * part->sb->dir_entry_size;
	
	/* 在父目录中添加自己的目录项 */
	struct llfs_dir_entry  new_dir_entry;
	memset(&new_dir_entry, 0, sizeof(struct llfs_dir_entry ));
	create_dir_entry(dentry->d_name, inode_no, FT_DIRECTORY, &new_dir_entry);
	memset(io_buf, 0, SECTOR_SIZE * 2);	 // 清空io_buf
	if (!sync_dir_entry(part, parent_dir, &new_dir_entry, io_buf)) {
		printk("sys_mkdir: sync_dir_entry to disk failed!\n");
		rollback_step = 2;
		goto rollback;
	}
	
	memset(io_buf, 0, SECTOR_SIZE * 2);
	inode_sync(part, parent_dir->inode, io_buf);
	memset(io_buf, 0, SECTOR_SIZE * 2);
	inode_sync(part, &new_dir_inode, io_buf);
	bitmap_sync(part, inode_no, INODE_BITMAP);
	kfree(io_buf);

	return 0;

/*创建文件或目录需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤 */
rollback:	     // 因为某步骤操作失败而回滚
	switch (rollback_step) {
		case 2:
		bitmap_set(&part->inode_bitmap, inode_no, 0);	 // 如果新文件的inode创建失败,之前位图中分配的inode_no也要恢复 
		case 1:
		/* 关闭所创建目录的父目录 */
		dir_close(parent_dir);
		break;
	}
	kfree(io_buf);
	return -1;
}

/* 格式化分区,也就是初始化分区的元信息,创建文件系统 */
static void partition_format(struct partition* part) {
/* 为方便实现,一个块大小是一扇区 */
	uint32_t boot_sector_sects = 1;	  
	uint32_t super_block_sects = 1;
	uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);	   // I结点位图占用的扇区数.最多支持4096个文件
	uint32_t inode_table_sects = DIV_ROUND_UP(((sizeof(struct llfs_inode) * MAX_FILES_PER_PART)), SECTOR_SIZE);
	uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
	uint32_t free_sects = part->sec_cnt - used_sects;  

	uint32_t block_bitmap_sects;
	block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
	/* block_bitmap_bit_len是位图中位的长度,也是可用块的数量 */
	uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects; 
	block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR); 
	
	/* 超级块初始化 */
	struct llfs_super_block sb;
	sb.magic = 0x19590318;
	sb.sec_cnt = part->sec_cnt;
	sb.inode_cnt = MAX_FILES_PER_PART;
	sb.part_lba_base = part->start_lba;
	
	sb.block_bitmap_lba = sb.part_lba_base + 2;	 // 第0块是引导块,第1块是超级块
	sb.block_bitmap_sects = block_bitmap_sects;
	
	sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
	sb.inode_bitmap_sects = inode_bitmap_sects;
	
	sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
	sb.inode_table_sects = inode_table_sects; 
	
	sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
	sb.root_inode_no = 0;
	sb.dir_entry_size = sizeof(struct llfs_dir_entry );
	
	printk("%s info:\n", part->name);
	printk("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   inode_cnt:0x%x\n   block_bitmap_lba:0x%x\n   block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   inode_table_sectors:0x%x\n   data_start_lba:0x%x\n", sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt, sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects, sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);
	
	struct disk* hd = part->my_disk;

	ide_write(hd, part->start_lba + 1, &sb, 1);
	printk("   super_block_lba:0x%x\n", part->start_lba + 1);

/* 找出数据量最大的元信息,用其尺寸做存储缓冲区*/
	uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
	buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
	uint8_t* buf = (uint8_t*)kalloc(buf_size);	// 申请的内存由内存管理系统清0后返回
   
	/* 初始化块位图block_bitmap */
	buf[0] |= 0x01;       // 第0个块预留给根目录,位图中先占位
	uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
	uint8_t  block_bitmap_last_bit  = block_bitmap_bit_len % 8;
	uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);	     // last_size是位图所在最后一个扇区中，不足一扇区的其余部分
	
	/* 1 先将位图最后一字节到其所在的扇区的结束全置为1,即超出实际块数的部分直接置为已占用*/
	memset(&buf[block_bitmap_last_byte], 0xff, last_size);
	
	/* 2 再将上一步中覆盖的最后一字节内的有效位重新置0 */
	uint8_t bit_idx = 0;
	while (bit_idx <= block_bitmap_last_bit) {
		buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
	}
	ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

   /* 先清空缓冲区*/
	memset(buf, 0, buf_size);
	buf[0] |= 0x1;      // 第0个inode分给了根目录
	/* 由于inode_table中共4096个inode,位图inode_bitmap正好占用1扇区,
		* 即inode_bitmap_sects等于1, 所以位图中的位全都代表inode_table中的inode,
		* 无须再像block_bitmap那样单独处理最后一扇区的剩余部分,
		* inode_bitmap所在的扇区中没有多余的无效位 */
	ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

 /* 准备写inode_table中的第0项,即根目录所在的inode */
	memset(buf, 0, buf_size);  // 先清空缓冲区buf
	struct llfs_inode* i = (struct llfs_inode*)buf; 
	i->i_size = sb.dir_entry_size * 2;	 // .和..
	i->i_no = 0;   // 根目录占inode数组中第0个inode
	i->i_sectors[0] = sb.data_start_lba;	     // 由于上面的memset,i_sectors数组的其它元素都初始化为0 
	ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);
   
	/* 写入根目录的两个目录项.和.. */
	memset(buf, 0, buf_size);
	struct llfs_dir_entry * p_de = (struct llfs_dir_entry *)buf;
	
	/* 初始化当前目录"." */
	memcpy(p_de->filename, ".", 1);
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;
	p_de++;
	
	/* 初始化当前目录父目录".." */
	memcpy(p_de->filename, "..", 2);
	p_de->i_no = 0;   // 根目录的父目录依然是根目录自己
	p_de->f_type = FT_DIRECTORY;
	
	/* sb.data_start_lba已经分配给了根目录,里面是根目录的目录项 */
	ide_write(hd, sb.data_start_lba, buf, 1);
	
	printk("   root_dir_lba:0x%x\n", sb.data_start_lba);
	printk("%s format done\n", part->name);
	kfree(buf);
}

int search_file(struct partition *part, const char* pathname, struct path_search_record* searched_record) {
	printk("invalid func\n");
	return 0;
}

/* 返回路径深度,比如/a/b/c,深度为3 */
int32_t path_depth_cnt(char* pathname) {
   ASSERT(pathname != NULL);
   char* p = pathname;
   char name[MAX_FILE_NAME_LEN];       // 用于path_parse的参数做路径解析
   uint32_t depth = 0;

   /* 解析路径,从中拆分出各级名称 */ 
   p = path_parse(p, name);
   while (name[0]) {
      depth++;
      memset(name, 0, MAX_FILE_NAME_LEN);
      if (p) {	     // 如果p不等于NULL,继续分析路径
	p  = path_parse(p, name);
      }
   }
   return depth;
}


/* 目录打开成功后返回目录指针,失败返回NULL */
struct llfs_dir* sys_opendir(const char* name) {
	struct partition *part;
	ASSERT(strlen(name) < MAX_PATH_LEN);
	/* 如果是根目录'/',直接返回&root_dir */
	if (name[0] == '/' && (name[1] == 0 || name[0] == '.')) {
		return &root_dir;
	}
	
	/* 先检查待打开的目录是否存在 */
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	int inode_no = search_file(part, name, &searched_record);
	struct llfs_dir* ret = NULL;
	if (inode_no == -1) {	 // 如果找不到目录,提示不存在的路径 
		printk("In %s, sub path %s not exist\n", name, searched_record.searched_path); 
	} else {
		if (searched_record.file_type == FT_REGULAR) {
			printk("%s is regular file!\n", name);
		} else if (searched_record.file_type == FT_DIRECTORY) {
			ret = dir_open(part, inode_no);
		}
	}
	dir_close(searched_record.parent_dir);
	return ret;
}


/* 在buf中填充文件结构相关信息,成功时返回0,失败返回-1 */
uint32_t sys_stat(const char* path, struct stat* buf) {
	struct partition *part;
	/* 若直接查看根目录'/' */
	if (!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")) {
		buf->st_filetype = FT_DIRECTORY;
		buf->st_ino = 0;
		buf->st_size = root_dir.inode->i_size;
		return 0;
	}
	
	int32_t ret = -1;	// 默认返回值
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));   // 记得初始化或清0,否则栈中信息不知道是什么
	int inode_no = search_file(part,path, &searched_record);
	if (inode_no != -1) {
		struct llfs_inode* obj_inode = inode_open(part, inode_no);   // 只为获得文件大小
		buf->st_size = obj_inode->i_size;
		inode_close(obj_inode);
		buf->st_filetype = searched_record.file_type;
		buf->st_ino = inode_no;
		ret = 0;
	} else {
		printk("sys_stat: %s not found\n", path);
	}
	dir_close(searched_record.parent_dir);
	return ret;
}