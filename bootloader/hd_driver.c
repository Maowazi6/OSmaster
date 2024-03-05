#define SECTSIZE 512
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned char uchar;

static inline uchar inb(ushort port){
	uchar data;
	asm volatile("in %1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void insl(uint port, void *addr, uint cnt){
	asm volatile("cld; rep insl" :
               "=D" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "memory", "cc");
}

static inline void outb(ushort port, uchar data){
	asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

void waitdisk(void){
	while((inb(0x1F7) & 0xC0) != 0x40);
}

void readsect(void *dst, uint offset){
	waitdisk();
	outb(0x1F2, 1);   
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20); 
	
	waitdisk();
	insl(0x1F0, dst, SECTSIZE/4);
}

void readseg(uint addr, uint cnt, uint offset){
	uint dest = addr;
	for(uint i = 0;i<cnt ; dest += SECTSIZE, i++, offset++){
		readsect(dest, offset);
	}
}