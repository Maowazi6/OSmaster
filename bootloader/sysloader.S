#这个文件主要是读取一些参数，因为进入保护模式之后bios就用不了了，所以要先读取一些参数
#然后开A20总线，进入保护模式，然后读取一些参数最后跳转到内核开始执行
#因为开启了保护模式，所以需要一个临时的gdt表，我选择直接平坦模式就3个描述符就行了，后面开启分页
#目前堆栈还是在0x7bff一个临时的堆栈，进内核之后重新分配
#这个文件的gdt在后面还是要初始化的，这里只是一个临时的

#include "realmode_def.h"
.code16 
	#新地点，新初始化
	#这里因为被重定位了，代码内的基地址就是0x8000,访问内存数据的话就是cs:offset所以cs设置为0
	mov $0, %ax
	mov %ax, %ds
	mov $ARDS_BUF_SEG, %ax
	mov %ax, %es
	mov $0, %di
#内存探测
do_int_e820:
	xor %ebx, %ebx		      #第一次调用时，ebx值要为0
	mov $0x534d4150, %edx 	  #edx只赋值一次，循环体中不会改变
int_e820_loop:
	mov $20, %ecx			  #ARDS一个20字节	
	mov $0x0000e820, %eax	  #BIOS子中断号
	int $0x15
	jc  do_int_e801
	incw num_ards
	add %cx, %di
	cmp $0, %ebx
	jnz int_e820_loop
	
get_max_ards_32bit:
	decw num_ards    
	movw num_ards, %cx
	movl $ARDS_BUF_OFF, %edx
	movl (%edx),%eax
	movl %eax, PARAMS_MEM_BASE
	addl $8, %edx
	movl (%edx), %eax
	addl $20, %edx
	movl (%edx), %ebx
do_cmp:
	cmp %eax, %ebx
	jc do_next
	movl %ebx, %eax
	movl -8(%edx), %ebx
	movl %ebx, PARAMS_MEM_BASE
do_next:
	addl $20, %edx
	movl (%edx), %ebx
	loop do_cmp

	jmp keep_memsize
	
#最后结果(当时用的32m):总的来说，最长的那一段基本上就是要找的值
#基地址       低32			 高32		  限长        低32         高32		     Type
#			00000000       00000000  				0009f000     00000000       00000001
#		    0009f000       00000000                 00001000     00000000       00000002	
#			000e8000       00000000                 00018000     00000000       00000002       
#			00100000       00000000					01ef0000     00000000       00000001	！大约30m
#			01ff0000       00000000                 00010000     00000000       00000003
#			fffc0000       00000000                 00040000     00000000       00000002

do_int_e801:
	#int $0x15
	#此时AX代表16m一下的内存，单位为1kb
	#BX为16M以上的内存，单位为64kb
keep_memsize:
	movl %eax, PARAMS_MEM_BASE+4
	
_goto_protected:
	cli
#进入保护模式  开A20  将cr0的pe位(第0位)置1，lgdt
	xorw    %ax,%ax             
	movw    %ax,%ds             
	movw    %ax,%es                        
#(经典的方法是写8042控制器，写0xdf到0x60端口)
#开A20总线(Fast Gate A20)由于键盘的控制器很慢(甚至有的微机是没有8042控制器的)，所以使用这种方法，
#但是有可能会产生冲突，还有一种方法就是读0xee开A20，写0xee关A20
	inb     $0x92, %al	# open A20 line(Fast Gate A20).
	orb     $2, %al
	outb    %al, $0x92
	
#加载gdt进入保护模式
	lgdt    set_lgdt
	movl    %cr0, %eax
	orl     $1, %eax
	movl    %eax, %cr0
	ljmp    $(1<<3), $_start32			#这个地方ljmp需要强制刷新流水线，重新加载选择子，然后就不用再被实模式下的分段所困扰了  0x8087
	
.code32  
_start32:
	movw    $(2<<3), %ax   
	movw    %ax, %ds               
	movw    %ax, %es               
	movw    %ax, %ss               
	movw    $0, %ax                
	movw    %ax, %fs               
	movw    %ax, %gs    
	
	
	
##加载内核
#hd_load_system:
#	#第1步：设置要读取的扇区数
#	mov $0x1f2, %dx
#	mov $SYS_SECTORS, %ax			#读600个扇区
#	out %al, %dx        			#参考out %0,%1" : : "a" (data), "d" (port)
#	#第2步：将LBA地址存入0x1f3 ~ 0x1f6
#	#LBA地址7~0位写入端口0x1f3
#	mov $0x1f3, %dx
#	mov $SYS_START_SECTOR, %eax
#	out %al, %dx
#	#LBA地址15~8位写入端口0x1f4
#	mov $8, %cx
#	shr %cl, %eax
#	mov $0x1f4, %dx
#	out %al, %dx
#	#LBA地址23~16位写入端口0x1f5
#	shr %cl, %eax
#	mov $0x1f5, %dx
#	out %al, %dx
#	
#	shr %cl, %eax
#    and $0x0f, %al	   		#lba第24~27位
#    or  $0xe0, %al	   		#设置7～4位为1110,表示lba模式
#    mov $0x1f6, %dx
#    out %al, %dx
#	
#	#第3步：向0x1f7端口写入读命令，0x20 
#    mov $0x1f7, %dx
#    mov $0x20, %al                        
#    out %al, %dx
#	#第4步：检测硬盘状态
#not_ready:
#    #同一端口，写时表示写入命令字，读时表示读入硬盘状态
#    in %dx, %al			#"in %1,%0" : "=a" (data) : "d" (port)
#    and $0x88, %al	   	#第4位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙
#    cmp $0x08, %al
#    jnz not_ready	   	#若未准备好，继续等。
#
#	#第5步：从0x1f0端口读数据
#    mov $SYS_SECTORS, %eax
#    mov $256, %dx
#    mul %edx	
#    mov %eax, %ecx	   #一个扇区有512字节，每次读入一个字，
#	#共需di*512/2次，所以di*256
#    mov $0x1f0, %dx
#	mov $ELF_BASE, %ebx
#go_on_read:
#    in %dx, %ax
#    mov %ax, (%ebx)
#    addl $2, %ebx		  
#    loop go_on_read
	
	
	pushl $SYS_START_SECTOR
	pushl $SYS_SECTORS
	pushl $ELF_BASE
	call readseg
	add $12 ,%esp
	
	jmp kernel_start
	
	
	
	
#载入完内核之后，设置临时页表，并且加载页表
kernel_start:
	movw    $(2<<3), %ax   
	movw    %ax, %ds               
	movw    %ax, %es               
	movw    %ax, %ss               
	movw    $0,  %ax                
	movw    %ax, %fs               
	movw    %ax, %gs 

setup_pagedir:
	movl $1024, %ecx 
	movl $PAGES_DITECTORY, %eax
clear_page_dir:
	movl $0, (%eax)
	addl $4, %eax
	loop clear_page_dir
create_pde:				    					#临时的页表，只需要映射两个作为  entry到 内核初始化页表之前这一段代码的临时页表
	movl $PAGES_DITECTORY, %eax 
	addl $0x1000, %eax 			     			# 此时eax为第一个页表的位置及属性 
	movl %eax, %ebx			    				# 此处为ebx赋值，是为create_pte做准备，ebx为基址。
	orl  $PG_US_U | PG_RW_W | PG_P, %eax 	    # 页目录项的属性RW和P位为1,US为1,表示用户属性,所有特权级别都可以访问.
	movl %eax, (PAGES_DITECTORY + 0x0)          # 第一个页表，因为当前还处在 没有映射的部分，所以需要这一项在加载页表之后使程序仍然能够正常运行
	movl %eax, (PAGES_DITECTORY + 0xc00)        # 进入system之后的用到的页表，内核用0xc0000000这个虚拟地址来链接
#创建第一个4mb页，一个临时页框
	movl $1024,%ecx				     
	movl $0   ,%esi
	movl $PG_US_U | PG_RW_W | PG_P, %edx   
create_first_pte:				    
	leal (%ebx,%esi,4),%edi	
	movl %edx, (%edi)
	addl $4096,%edx
	incl %esi
	loop create_first_pte
	
#加载页表
	movl    $PAGES_DITECTORY, %eax
	movl    %eax, %cr3
	movl    %cr0, %eax
	orl     $(CR0_PG|CR0_WP), %eax
	movl    %eax, %cr0
	movl 	$STACK_TEMP, %esp      #这个时候esp应该用虚拟地址了，不然后面解除低4M对低4M内存的映射的时候就缺页异常了
	

#加载elf
kernel_init:
	xor %eax, %eax
	xor %ebx, %ebx		
	xor %ecx, %ecx		
	xor %edx, %edx		
	movl $ELF_BASE, %eax
	movw 42(%eax) ,%dx
	movl 28(%eax) ,%ebx 
						
	addl $ELF_BASE,%ebx
	movw 44(%eax), %cx
each_segment:
	movl 0(%ebx), %eax 
    cmpb $PT_NULL, %eax  
	je PTNULL

  
	pushl 16(%ebx)		 
	movl 4(%ebx), %eax	  
	addl $ELF_BASE, %eax  
	pushl %eax				  
	pushl 8(%ebx)			 
	call mem_cpy				  
	addl $12, %esp				  
PTNULL:
   addl %edx, %ebx			  
   loop each_segment


#这里开始进入内核	PARAMS_MEM_BASE是0xfff0这个地方，表示可用内存段的基地址，但是有个问题就是链接文件已经确定了装载地址，所以不能这样做，
#内核的加载地址应该是一个写死的地址,即使用的是一个虚拟地址，但还是要设置页表映射，这个页表实际上也是写死的

	movl $ELF_BASE, %ebx    #这个地方已经加载页表了，里面有两项，所以是否需要加上P2V都是可以的
	movl 24(%ebx), %eax
	jmp *%eax   			#加上*号表示用寄存器里面的地址来跳转是间接跳转，因为指令码太长   0x81c9
	
mem_cpy:		      
   cld
   pushl %ebp
   movl %esp, %ebp
   pushl %ecx		  
   movl 8(%ebp) , %edi 	   # dst
   movl 12(%ebp), %esi	   # src
   movl 16(%ebp), %ecx	   # size
   rep movsb		   # 逐字节拷贝

   pop %ecx		
   pop %ebp
   ret
   
#print_hex:
#	mov $4,%cx
#	mov %ax,%dx
#
#print_digit:
#	rol $4,%dx	#循环以使低4位用上，高4位移至低4位
#	mov $0xe0f,%ax #ah ＝ 请求的功能值，al = 半个字节的掩码
#	and %dl,%al
#	add $0x30,%al
#	cmp $0x3a,%al
#	jl outp
#	add $0x07,%al
#
#outp:
#	int $0x10
#	loop print_digit
#	ret
##打印回车换行
#print_nl:
#	mov $0xe0d,%ax
#	int $0x10
#	mov $0xa,%al
#	int $0x10
#	ret

#段限长是20位但是4kb大小由第54位D/B位决定
# |---------------------------------------------|--------------------------------|
# |63~56  |55  |54   |53  |52    |   51~48      | 47  |46~45  |44  |43~40  |39~32|
# |基址   |G   |D/B  |L   |AVL   | 限长(19~16)  | P   | DPL   |S   |TYPE   |基址 |
# |---------------------------------------------+--------------------------------|
# |              31~16                          |              15~0              |
# |            基址(15~0)                       |           限长(15~0)           |
# |---------------------------------------------|--------------------------------|
.p2align 2                                # force 4 byte alignment
#临时gdt  平坦模式  不用分段，直接分页
gdt_table:
  # null 为避免忘记初始化选择子而误选到了GDT中的第0个描述符 cpu在执行是会产生异常
  #这就确保了只有经过显式地初始化后才能从gdt中检索描述符
  .byte 0, 0, 0, 0, 0, 0, 0, 0					  
  # code
  .word 0xffff, 0x0 
  .byte 0x0, 0x9a, 0xcf, 0x0
  # data  
  .word 0xffff, 0x0
  .byte 0x0, 0x92, 0xcf, 0x0  

#gdt表由16位段限长和32位段基址构成
set_lgdt:
  .word   (set_lgdt - gdt_table - 1)             # gdt表长度   这里是0~3*8-1
  .long   gdt_table                             # 

num_ards:
	.word 0