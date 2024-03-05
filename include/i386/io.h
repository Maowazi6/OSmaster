#ifndef _IO_H
#define _IO_H
#include "types.h"
#include "cga.h"

static inline uint32_t readeflags(void) {
  uint32_t eflags;
  asm volatile("pushfl; popl %0" : "=r" (eflags));
  return eflags;
}


static inline uint8_t inb(uint16_t port) {
  uint8_t data;

  asm volatile("inb %w1,%b0" : "=a" (data) : "d" (port));
  return data;
}

static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
   asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}

/* 将从端口port读入的word_cnt个字写入addr */
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt) {
   asm volatile ("cld; rep insw" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");
}

static inline void insl(int port, void *addr, int cnt) {
  asm volatile("cld; rep insl" :
               "=D" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "memory", "cc");
}


static inline void outb(uint16_t port, uint8_t data) {
  asm volatile("outb %b0,%w1" : : "a" (data), "d" (port));
}


static inline void outw(uint16_t port, uint16_t data) {
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}


static inline void outsl(int port, const void *addr, int cnt) {
  asm volatile("cld; rep outsl" :
               "=S" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "cc");
}


static inline void stosb(void *addr, int data, int cnt) {
  asm volatile("cld; rep stosb" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}


static inline void stosl(void *addr, int data, int cnt) {
  asm volatile("cld; rep stosl" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}



static inline uint32_t get_eax(){
	uint32_t temp;
	asm volatile("pushl %%eax; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_ebx(){
	uint32_t temp;
	asm volatile("pushl %%ebx; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_ecx(){
	uint32_t temp;
	asm volatile("pushl %%ecx; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_edx(){
	uint32_t temp;
	asm volatile("pushl %%edx; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_esp(){
	uint32_t temp;
	asm volatile("pushl %%esp; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_ebp(){
	uint32_t temp;
	asm volatile("pushl %%ebp; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_esi(){
	uint32_t temp;
	asm volatile("pushl %%esi; popl %0" : "=r" (temp));
	return temp;
}

static inline uint32_t get_edi(){
	uint32_t temp;
	asm volatile("pushl %%edi; popl %0" : "=r" (temp));
	return temp;
}


#endif