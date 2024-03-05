include ../Makefile.header
#CFLAGS+= -MD

OBJS = hd_driver.o

bootloader:$(OBJS) bootblock  sysloader 
	
bootblock:
	@$(CC) $(CFLAGS)  -O -nostdinc -I. -c bootloader_hd.S
	@$(LD) $(LDFLAGS) -o bootloader -Ttext 0 bootloader_hd.o 	
	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary bootloader	

sysloader:
	@$(CC) $(CFLAGS) -O -nostdinc -I. -c sysloader.S
	@$(LD) $(LDFLAGS) -o sysloader -Ttext 0x8000 sysloader.o hd_driver.o	
	@$(OBJCOPY) -R .pdr -R .comment -R.note -S -O binary sysloader	

#entry:
#	$(CC) $(CFLAGS) -O -nostdinc -I. -c entry.S
	
clean:
	rm *.d *.o bootloader sysloader


