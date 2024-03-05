include ../../Makefile.header
LDFLAGS += -r
LIB = -I ../../include/ -I ../../include/i386/ -I ../../include/kernel/ -I ../../fs/ -I ../../fs/linux-like/
CFLAGS+= -MD $(LIB)

OBJS =  sync.o \
		kproc.o \
		fork.o \
		wait_exit.o \
		exec.o
	
all:$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o proc.o
	@sync
	
.PHONY: all