#include "string.h"
#include "types.h"
#include "uassert.h"


/* 将dst_起始的size个字节置为value */
void memset(void* dst_, uint8_t value, uint32_t size) {
	assert(dst_ != NULL);
	uint8_t* dst = (uint8_t*)dst_;
	while (size-- > 0)
		*dst++ = value;
}

int memcmp(const void *v1, const void *v2, uint32_t n) {
	const uint8_t *s1, *s2;
	
	s1 = v1;
	s2 = v2;
	while(n-- > 0){
		if(*s1 != *s2)
			return *s1 - *s2;
		s1++, s2++;
	}
	
	return 0;
}

void* memmove(void *dst, const void *src, uint32_t n) {
	const char *s;
	char *d;
	
	s = src;
	d = dst;
	if(s < d && s + n > d){
		s += n;
		d += n;
		while(n-- > 0)
		*--d = *--s;
	} else
		while(n-- > 0)
		*d++ = *s++;
	
	return dst;
}

/* 将src_起始的size个字节复制到dst_ */
void memcpy(void* dst_, const void* src_, uint32_t size) {
	assert(dst_ != NULL && src_ != NULL);
	uint8_t* dst = dst_;
	const uint8_t* src = src_;
	while (size-- > 0)
		*dst++ = *src++;
}

int strncmp(const char *p, const char *q, uint32_t n) {
	while(n > 0 && *p && *p == *q)
		n--, p++, q++;
	if(n == 0)
		return 0;
	return (uint8_t)*p - (uint8_t)*q;
}

char* strncpy(char *s, const char *t, int n) {
	char *os;
	
	os = s;
	while(n-- > 0 && (*s++ = *t++) != 0)
		;
	while(n-- > 0)
		*s++ = 0;
	return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char* safestrcpy(char *s, const char *t, int n) {
	char *os;
	
	os = s;
	if(n <= 0)
		return os;
	while(--n > 0 && (*s++ = *t++) != 0)
		;
	*s = 0;
	return os;
}

int strlen(const char *s) {
	int n;
	
	for(n = 0; s[n]; n++)
		;
	return n;
}

/* 将字符串从src_复制到dst_ */
char* strcpy(char* dst_, const char* src_) {
	//assert(dst_ != NULL && src_ != NULL);
	char* r = dst_;		       // 用来返回目的字符串起始地址
	while((*dst_++ = *src_++));
	return r;
}

/* 比较两个字符串,若a_中的字符大于b_中的字符返回1,相等时返回0,否则返回-1. */
int8_t strcmp (const char* a, const char* b) {
	assert(a != NULL && b != NULL);
	while (*a != 0 && *a == *b) {
		a++;
		b++;
	}
	
	return *a < *b ? -1 : *a > *b;
}

/* 从左到右查找字符串str中首次出现字符ch的地址(不是下标,是地址) */
char* strchr(const char* str, const uint8_t ch) {
	assert(str != NULL);
	while (*str != 0) {
		if (*str == ch) {
			return (char*)str;	    // 需要强制转化成和返回值类型一样,否则编译器会报const属性丢失,下同.
		}
		str++;
	}
	return NULL;
}

/* 从后往前查找字符串str中首次出现字符ch的地址(不是下标,是地址) */
char* strrchr(const char* str, const uint8_t ch) {
	assert(str != NULL);
	const char* last_char = NULL;
	/* 从头到尾遍历一次,若存在ch字符,last_char总是该字符最后一次出现在串中的地址(不是下标,是地址)*/
	while (*str != 0) {
		if (*str == ch) {
			last_char = str;
		}
		str++;
	}
	return (char*)last_char;
}

/* 将字符串src_拼接到dst_后,将回拼接的串地址 */
char* strcat(char* dst_, const char* src_) {
	assert(dst_ != NULL && src_ != NULL);
	char* str = dst_;
	while (*str++);
	--str;      // 别看错了，--str是独立的一句，并不是while的循环体
	while((*str++ = *src_++));	 // 当*str被赋值为0时,此时表达式不成立,正好添加了字符串结尾的0.
	return dst_;
}

/* 在字符串str中查找指定字符ch出现的次数 */
uint32_t strchrs(const char* str, uint8_t ch) {
	assert(str != NULL);
	uint32_t ch_cnt = 0;
	const char* p = str;
	while(*p != 0) {
		if (*p == ch) {
			ch_cnt++;
		}
		p++;
	}
	return ch_cnt;
}
