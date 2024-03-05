#考虑到，软盘的使用方法，由于在进入保护模式之后无法使用bios，所以要是想将system移动到0x100000处就得使用IDE来读取，
#目前还不清楚软盘能不能按照硬盘来读取，所以换成了硬盘驱动，然后在进入保护模式之后才加载system

#include "realmode_def.h"
.code16
#前512字节直接指定为text段，声明数据段等段的意义不大
.global _start, _end, begtext, begdata, begbss, endtext, enddata, endbss
.text
begtext:
.data
begdata:
.bss
begbss:


.text  #开始
#0x0
ljmp    $BOOTSEG, $_start

_start:
	mov	$0, %ax		
	mov	%ax, %ds
	mov	%ax, %es
	#设置堆栈  0x7bff-0x500
	mov $STACK_BASE, %ax
	mov %ax, %ss
	mov	$0x00ff, %sp

#这个是硬盘的驱动
hd_load_setup:
	#第1步：设置要读取的扇区数
	mov $0x1f2, %dx
	mov $SETUP_SECTIORS_LEN, %ax			#读4个扇区
	out %al, %dx        #参考out %0,%1" : : "a" (data), "d" (port)
	#第2步：将LBA地址存入0x1f3 ~ 0x1f6
	#LBA地址7~0位写入端口0x1f3
	mov $0x1f3, %dx
	mov $START_SECTORS_BASE, %eax
	out %al, %dx
	#LBA地址15~8位写入端口0x1f4
	mov $8, %cx
	shr %cl, %eax
	mov $0x1f4, %dx
	out %al, %dx
	#LBA地址23~16位写入端口0x1f5
	shr %cl, %eax
	mov $0x1f5, %dx
	out %al, %dx
	
	shr %cl, %eax
    and $0x0f, %al	   		#lba第24~27位
    or  $0xe0, %al	   		#设置7～4位为1110,表示lba模式
    mov $0x1f6, %dx
    out %al, %dx
	
	#第3步：向0x1f7端口写入读命令，0x20 
    mov $0x1f7, %dx
    mov $0x20, %al                        
    out %al, %dx
	#第4步：检测硬盘状态
not_ready:
    #同一端口，写时表示写入命令字，读时表示读入硬盘状态
    in %dx, %al			#"in %1,%0" : "=a" (data) : "d" (port)
    and $0x88, %al	   	#第4位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙
    cmp $0x08, %al
    jnz not_ready	   	#若未准备好，继续等。

	#第5步：从0x1f0端口读数据
    mov $SETUP_SECTIORS_LEN, %ax
    mov $256, %dx
    mul %dx
    mov %ax, %cx	   #一个扇区有512字节，每次读入一个字，
	#共需di*512/2次，所以di*256
    mov $0x1f0, %dx
	mov $SETUP_LOAD, %bx
go_on_read:
    in %dx, %ax
    mov %ax, (%bx)
    add $2, %bx		  
    loop go_on_read



load_system:
	call print_msg
	ljmp	$SETUP_SEG, $0  #进入setup开始执行了  0x7c6c


print_msg:
	mov $0x07c0, %ax
	mov %ax, %es
	mov	$0x03, %ah		
	xor	%bh, %bh
	int	$0x10
	mov	$23, %cx
	mov	$0x0007, %bx		
	mov $msg1, %bp
	mov	$0x1301, %ax		
	int	$0x10
	ret
msg1:
	.byte 13,10
	.ascii "OS is booting ..."
	.byte 13,10,13,10

_end:
	.org  510
	.word 0xAA55

.text
endtext:
.data
enddata:
.bss
endbss:
