#include "keyboard.h"
#include "types.h"
#include "interrupt.h"
#include "io.h"
#include "general.h"
#include "ioqueue.h"
#include "console.h"


#define KBD_BUF_PORT 0x60	 // 键盘buffer寄存器端口号为0x60

/* 用转义字符定义部分控制字符 */
#define esc		'\033'	 // 八进制表示字符,也可以用十六进制'\x1b'
#define backspace	'\b'
#define tab		'\t'
#define enter		'\r'
#define delete		'\177'	 // 八进制表示字符,十六进制为'\x7f'

/* 以上不可见字符一律定义为0 */
#define char_invisible	0
#define ctrl_l_char		char_invisible
#define ctrl_r_char		char_invisible
#define shift_l_char	char_invisible
#define shift_r_char	char_invisible
#define alt_l_char		char_invisible
#define alt_r_char		char_invisible
#define caps_lock_char	char_invisible

/* 定义控制字符的通码和断码 */
#define shift_l_make	0x2a
#define shift_r_make 	0x36 
#define alt_l_make   	0x38
#define alt_r_make   	0xe038
#define alt_r_break   	0xe0b8
#define ctrl_l_make  	0x1d
#define ctrl_r_make  	0xe01d
#define ctrl_r_break 	0xe09d
#define caps_lock_make 	0x3a

struct ioqueue kbd_buf;	   // 定义键盘缓冲区

//控制键的全局变量
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

/*通码映射表*/
static char keymap[][2] = {
{0,	0},		
{esc,	esc},		
{'1',	'!'},		
{'2',	'@'},		
{'3',	'#'},		
{'4',	'$'},		
{'5',	'%'},		
{'6',	'^'},		
{'7',	'&'},		
{'8',	'*'},		
{'9',	'('},		
{'0',	')'},		
{'-',	'_'},		
{'=',	'+'},		
{backspace, backspace},	
{tab,	tab},		
{'q',	'Q'},		
{'w',	'W'},		
{'e',	'E'},		
{'r',	'R'},		
{'t',	'T'},		
{'y',	'Y'},		
{'u',	'U'},		
{'i',	'I'},		
{'o',	'O'},		
{'p',	'P'},		
{'[',	'{'},		
{']',	'}'},		
{enter,  enter},
{ctrl_l_char, ctrl_l_char},
{'a',	'A'},		
{'s',	'S'},		
{'d',	'D'},		
{'f',	'F'},		
{'g',	'G'},		
{'h',	'H'},		
{'j',	'J'},		
{'k',	'K'},		
{'l',	'L'},		
{';',	':'},		
{'\'',	'"'},		
{'`',	'~'},		
{shift_l_char, shift_l_char},	
{'\\',	'|'},		
{'z',	'Z'},		
{'x',	'X'},		
{'c',	'C'},		
{'v',	'V'},		
{'b',	'B'},		
{'n',	'N'},		
{'m',	'M'},		
{',',	'<'},		
{'.',	'>'},		
{'/',	'?'},
{shift_r_char, shift_r_char},	
{'*',	'*'},    	
{alt_l_char, alt_l_char},
{' ',	' '},		
{caps_lock_char, caps_lock_char}
};

/* 键盘中断处理程序 */
static void intr_keyboard_handler(void) {

	/* 这次中断发生前的上一次中断,以下任意三个键是否有按下 */
	bool ctrl_down_last = ctrl_status;	  
	bool shift_down_last = shift_status;
	bool caps_lock_last = caps_lock_status;

	bool break_code;
	uint16_t scancode = inb(KBD_BUF_PORT);

	if (scancode == 0xe0) { 
		ext_scancode = true;    // 打开e0标记
		return;
	}

	/* 如果上次是以0xe0开头,将扫描码合并 */
	if (ext_scancode) {
		scancode = ((0xe000) | scancode);
		ext_scancode = false;   // 关闭e0标记
	}   

	break_code = ((scancode & 0x0080) != 0);   // 获取break_code
   
	if (break_code) {//断码
		uint16_t make_code = (scancode &= 0xff7f);   // 得到其make_code(按键按下时产生的扫描码)
		if (make_code == ctrl_l_make || make_code == ctrl_r_make) {
			ctrl_status = false;
		} else if (make_code == shift_l_make || make_code == shift_r_make) {
			shift_status = false;
		} else if (make_code == alt_l_make || make_code == alt_r_make) {
			alt_status = false;
		} 
		return;  
	}else if ((scancode > 0x00 && scancode < 0x3b) || (scancode == alt_r_make) || (scancode == ctrl_r_make)) {//通码
		bool shift = false; 
		if((scancode < 0x0e) || (scancode == 0x29) || (scancode == 0x1a) || (scancode == 0x1b) || (scancode == 0x2b) \
		  || (scancode == 0x27) || (scancode == 0x28) || (scancode == 0x33) || (scancode == 0x34) || (scancode == 0x35)) {//处理字符  
			if (shift_down_last) {  
				shift = true;
			}
		} else {// 处理字母键
			if (shift_down_last && caps_lock_last) {  
				shift = false;
			} else if (shift_down_last || caps_lock_last) { 
				shift = true;
			} else {
				shift = false;
			}
		}
	
		uint8_t index = (scancode &= 0x00ff);  
		char cur_char = keymap[index][shift]; 
	
		/* 如果cur_char为可见字符,就加入键盘缓冲区中 */
		if (cur_char) {
			if ((ctrl_down_last && cur_char == 'l') || (ctrl_down_last && cur_char == 'u')) {
				cur_char -= 'a';
			}
			
			if (!ioq_full(&kbd_buf)) {
				ioq_putchar(&kbd_buf, cur_char);
			}
			return;
		}
	
		//处理不可见字符
		if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
			ctrl_status = true;
		} else if (scancode == shift_l_make || scancode == shift_r_make) {
			shift_status = true;
		} else if (scancode == alt_l_make || scancode == alt_r_make) {
			alt_status = true;
		} else if (scancode == caps_lock_make) {
			caps_lock_status = !caps_lock_status;
		}
	} else {
		cga_putstr("unknown key\n");
	}
}

/* 键盘初始化 */
void keyboard_init() {
   cga_putstr("keyboard init start\n");
   ioqueue_init(&kbd_buf);
   idt_register(0x21, intr_keyboard_handler);
   cga_putstr("keyboard init done\n");
}

