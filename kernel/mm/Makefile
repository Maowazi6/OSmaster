include ../../Makefile.header
LDFLAGS += -r
LIB = -I ../../include/ -I ../../include/i386/ -I ../../include/kernel/
CFLAGS+= -MD $(LIB)

OBJS =  kalloc.o \
		mm.o
		
all:$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o memory.o
	@sync
	
.PHONY: all