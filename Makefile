include Makefile.header
LIB = -I include/ -I include/i386/ -I include/kernel/ -I fs/linux-like -I app/
CFLAGS+= -MD $(LIB)
bootImage = kernel.img

OBJS = 	main.o \
		kernel/kernel.o
		
LDFLAGS += -T kernel.ld
boot:bootloader1 
	@dd if=bootloader/bootloader of=kernel.img bs=512 count=1 conv=notrunc
	@dd if=bootloader/sysloader seek=1 of=kernel.img conv=notrunc


kernel:$(OBJS)
	@$(LD) $(LDFLAGS)  -o kernel_img $(OBJS) 
	#@$(STRIP) kernel_img
	#@$(OBJCOPY) -O binary -R .note -R .comment kernel_img kernel_img
	
image:kernelimage boot kernel
	@dd if=kernel_img seek=5 of=kernel.img conv=notrunc
	
kernelimage:
	@dd if=/dev/zero of=kernel.img count=10000
	
bootloader1:
	@make -C bootloader

#disk:
#	@dd if=/dev/zero of=kernel.img count=10000

mcp:debg
	cp ./kernel.img /mnt/hgfs/share_dir

kernel/kernel.o:
	@make -C kernel

debg:
	objdump -d ./kernel_img > 1.txt
	cp ./1.txt /mnt/hgfs/share_dir

qemu:
	@qemu-system-x86_64 -drive file=kernel.img,index=0,media=disk,format=raw -drive file=fs.img,index=1,media=disk,format=raw -m 64

	
#qemu-debug:
#	@qemu-system-x86_64 -m 32M -boot a -hda image1 -s -S
	
.PHONY: image kernel





















start:
	@qemu-system-x86_64 -m 16M -boot a -fda boot

qemu-gdb:
	@qemu-system-i386 -m 16M \
	-boot a \
	-fda bootloader \
	-S \
	-s
	
