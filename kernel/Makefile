include ../Makefile.header
LIB = -I ../include/ -I ../include/i386/ -I ../include/kernel/ -I ../fs/linux-like/ -I ../fs/ -I ../app
CFLAGS+= -MD $(LIB)

LDFLAGS += -r

ARCH_OBJS = ../arch/i386/arch.o
			
INTERRUPT_OBJS = interrupt/intr.o
				 
DEVICE_OBJS = 	../device/device.o
				
MEM_OBJS = mm/memory.o
		   
SCHED_OBJS = sched/proc.o

LIB_OBJS = ../lib/klib.o

FS_OBJS = ../fs/osfs.o

APP_OBJS = ../app/cmd_shell.o \
		   ../app/buildin_cmd.o

OBJS = $(DEVICE_OBJS) $(INTERRUPT_OBJS) $(LIB_OBJS) $(MEM_OBJS) $(SCHED_OBJS) $(ARCH_OBJS) $(FS_OBJS) $(APP_OBJS)

kernel.o: all 
	$(LD) $(LDFLAGS) $(OBJS) -o kernel.o
	@sync

all:interrupt device lib mm sched arch fs app

arch:
	make -C ../arch/i386
interrupt:
	make -C interrupt
mm:
	make -C mm
sched:
	make -C sched
device:
	make -C ../device
lib:
	make -C ../lib
fs:
	make -C ../fs
app:
	make -C ../app

.PHONY: interrupt device lib mm sched arch fs app