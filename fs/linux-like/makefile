include ../../Makefile.header
LDFLAGS += -r
LIB = -I ../../include/ -I ../../include/i386/ -I ../../include/kernel/ -I ./
CFLAGS+= -MD $(LIB)

OBJS =  fs.o \
		inode.o \
		file.o \
		dir.o
	
all:$(OBJS) llinux
	
llinux:
	$(LD) $(LDFLAGS) $(OBJS) -o llinux_fs.o


.PHONY: all