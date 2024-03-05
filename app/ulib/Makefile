CFLAGS = -m32 -Wall -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers -fno-stack-protector
CC = gcc
AS	= as --32
LDFLAGS = -m elf_i386
AR	= ar
LIB = -I .

CFLAGS += $(LIB)

OBJS =  stdio.o \
		uassert.o \
		string.o \
		usyscall.o \
		crt.o
		


create_ulib: create_crt $(OBJS)
	$(AR) -cr libulib.a $(OBJS)

create_crt:
	$(AS) crt.S -o crt.o
	
.PHONY:create_ulib create_crt