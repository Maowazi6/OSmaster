include ../Makefile.header
LIB = -I ../include/ -I ../include/i386/ -I ../include/kernel/
CFLAGS+= -MD $(LIB)
LDFLAGS += -r

OBJS = 	string.o \
		assert.o \
		assert_user.o \
		list.o   \
		bitmap.o \
		stdio.o \
		ioqueue.o \
		usyscall.o \
		Hash_Bucket.o

all:$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o klib.o
	@sync
.PHONY: all