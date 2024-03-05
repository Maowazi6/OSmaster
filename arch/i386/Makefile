include ../../Makefile.header
LIB = -I ../../include/ -I ../../include/i386/ -I ../../include/kernel/
CFLAGS+= -MD $(LIB)
LDFLAGS += -r

OBJS =  idt_vectors.o \
		idt.o \
		gdt.o \
		pageinit.o \
		swtch.o \
		tss.o
		
		
all:idt_vectors swtch_obj $(OBJS) arch_obj

idt_vectors: idt_vectors.pl 
	./idt_vectors.pl > idt_vectors.S
	$(AS) idt_vectors.S -o idt_vectors.o
	
swtch_obj:swtch.S
	$(AS) swtch.S -o swtch.o

arch_obj:
	$(LD) $(LDFLAGS) $(OBJS) -o arch.o
	@sync

.PHONY: all idt_vectors swtch_obj arch_obj