include ../Makefile.header
LIB = -I ../include/ -I ../include/i386/ -I ../include/kernel/  -I ../fs/ -I ../fs/linux-like/
CFLAGS += -MD $(LIB)
LDFLAGS += -r

OBJS =  cmd_shell.o \
		buildin_cmd.o 


all: $(OBJS) create_ulib
	
newapp:
	gcc -m32 -Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes \
	   -Wsystem-headers -fno-stack-protector -I ./ulib -o print.o print.c
	ld -m elf_i386 print.o -lulib -L ./ulib -o print
	dd if=print of=../kernel.img \
	   bs=512 count=30 seek=1024 conv=notrunc

create_ulib:
	make -C ./ulib

.PHONY:all newapp