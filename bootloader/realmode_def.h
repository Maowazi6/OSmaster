// 0x7c00  bootloader  
// 0x8000  sysloader
// 0xfff0  往前存放的是一些硬件参数
// 0x500~0x7bff  目前作为临时栈
// 0x100000  开始的是内核
//bootloader.S
#define SETUP_LOAD  		0x8000	   		//setup被安装的地方
#define SETUP_SEG 			0x0800            
#define BOOTSEG       		0x07c0	            
#define STACK_BASE 			0x07b0	   		//栈区
#define SETUP_SECTIORS_LEN 	4   	
#define START_SECTORS_BASE 	1   	//注意这里chs的第一个扇区是1，而lba的第一个扇区是0开始计数，所以1表示第二个扇区

//sysloader.S
#define SETUPCODE        	0x0800	   	//setup被安装的地方
//这个东西当时
#define PARAMS_MEM_BASE  	0xfff0   	//这个地方放的是内存参数，前4字节是基址，后四字节是限长，为什么放在这个地方呢，这是一个临时地方，后面可以挪，主要还是实模式下段不好改
#define ARDS_BUF_SEG     	0x0fe0      // 由于这个ARDS大小可能不固定而且会很大，至少都得有120字节，所以我留了512字节给他
#define ARDS_BUF_OFF     	0xfe00	
#define SYS_SECTORS      	200         //暂定200扇区是100kb我觉得这个大小远大于我内核的大小 
#define SYS_START_SECTOR 	5
#define SYS_BASE			0x100000
#define ELF_BASE			0x10000     //一开始将内核加载到这个地方，但是这个地方比较小，以后如果内核扩大的话这个地方就要改动，内核加载要快，
										//不能一个扇区一个扇区地读，这里是一次性读完，然后拷贝，缺点就是,占用空间太大，所以大的内核不能用这种方式
#define PT_NULL             0

#define PAGES_DITECTORY     0X1000
//第0位  	P位等于1表示该页存在
#define PG_P  		   		0x1      
//第1位		RW位意为读写位。若为1表示可读可写，若为0表示可读不可写
#define PG_RW_W	 			0x2     
//第3位为普通用户/超级用户位.若为 1 时，表示处于 User 级，任意级别（0,1,2,3）特权的程序都可以访问该页。
//若为0表示处于 Supervisor 级，特权级别为 3 的程序不允许访问该页，该页只允许特权级别为 0,1,2的程序可以访问。
#define PG_US_U	 			0x4 	
//PWT, Page-level Write-through，页写透，为1表示既写内存又写高速缓存
#define PG_PWT_UP         	0x8 
//PCD, Page-level Cache Disable,意为页级高速缓存禁止位.若为 1 表示该页启用高速缓存，为 0 表示禁止将该页缓存.这里默认设置0
#define PG_PCD_UP           0x10
//A, Accessed,意为访问位 。 若为 1 表示该页被 CPU 访问过(读或写)，由硬件自己设置
#define PG_A_UP				0x20
//D意为脏页位.当 CPU 对 一 个页面执行写操作时，就会设置对应页表项的D位为 l 。 此项
//仅针对页表项有效，并不会修改页目录项中的 D 位。
#define PG_D_UP				0x40
//PAT, Page Attribute Table ，意为页属性表位，能够在页面一级的粒度上设置内存属性。默认置为0表示4kb,为1是4MB
#define PG_PAT_UP			0x80
//G全局位，为1表示该页是全局的高速缓存一直保存，cr3被重新加载会清空TLB，置1后不会被清空，该页一直保存
#define PG_G_UP				0x100

#define CR0_PE          0x00000001      
#define CR0_WP          0x00010000      
#define CR0_PG          0x80000000   
#define KERNEL_BASE     0xc0000000
#define STACK_TEMP      0Xc0007bff   
#define ENTRY_V2P(address)    ((address)-KERNEL_BASE)
#define ENTRY_P2V(address)    ((address)+KERNEL_BASE)