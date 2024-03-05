include ../Makefile.header
LIB = -I ../include/ -I ../include/i386/ -I ../include/kernel/
CFLAGS+= -MD $(LIB)
LDFLAGS += -r

OBJS = 	cga.o \
		pic_8259A.o \
		timer_8253.o \
		console.o \
		ide.o \
		keyboard.o
		
all:$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o device.o
	@sync

.PHONY: all