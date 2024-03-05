#include "types.h"
#include "cga.h"
#include "string.h"
#include "mmlayout.h"
#include "io.h"

#define BACKSPACE 0x8
#define CRTPORT 0x3d4
#define CRTDATA 0x3d5

static uint16_t *crt = (uint16_t*)P2V(0xb8000);  // CGA memory


//更新显卡的两个pos寄存器
static void set_cursor (uint32_t pos){
	outb(CRTPORT, 14);
	outb(CRTDATA, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTDATA, pos);
}


void cga_putc(uint8_t c){
	int pos;
	
	outb(CRTPORT, 14);
	pos = inb(CRTDATA) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTDATA);
	
	if(c == '\n'||c == '\r')
		pos += 80 - pos%80;
	else if(c == '\b'){
		if(pos > 0) --pos;
	} else
		crt[pos++] = (c&0xff) | 0x0700;  // 黑底白字
	
	//if(pos < 0 || pos > 25*80)
		//panic("pos under/overflow");
	
	if((pos/80) >= 24){  //向上滚一行
		memmove(crt, crt+80, sizeof(crt[0])*23*80);
		pos -= 80;
		memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
	}
	
	set_cursor(pos);
	crt[pos] = ' ' | 0x0700;
}

//清屏
void cga_clear(){
	memset(crt, 0, sizeof(crt[0])*(25*80));
	set_cursor(0);
}


void cga_putstr(char *str){
	while(*str!='\0'){
		cga_putc(*str++);
	}
}

void put_int2hex(uint32_t num){
	uint32_t tmp = 0;
	uint32_t i = 0;
	cga_putstr("0x");
	for(i=0;i<8;i++){
		tmp = num & 0xf0000000;
		tmp >>= 28;
		if(tmp>=0&&tmp<=9){
			tmp+='0';
		}else{
			tmp-=10;
			tmp+='A';
		}
		cga_putc((uint8_t)tmp);
		num <<= 4;
	}
	cga_putstr("\n");
}