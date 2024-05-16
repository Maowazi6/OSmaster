include ../Makefile.header
LDFLAGS += -r
LIB = -I ../include/ -I ../include/i386/ -I ../include/kernel/ -I ./linux-like
CFLAGS+= -MD $(LIB)


OBJS =  pipe.o \
		./linux-like/llinux_fs.o \
		./vfs.o
all:support_fs $(OBJS) 
	$(LD) $(LDFLAGS) $(OBJS) -o osfs.o

support_fs:
	make -C ./linux-like/
	
.PHONY: all