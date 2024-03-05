#ifndef _STRING_H
#define _STRING_H
#include "types.h"

void* memset(void *dst, int c, uint32_t n);
//void memset(void* dst_, uint8_t value, uint32_t size);
int memcmp(const void *v1, const void *v2, uint32_t n);
void* memmove(void *dst, const void *src, uint32_t n);
void* memcpy(void *dst, const void *src, uint32_t n);
int strncmp(const char *p, const char *q, uint32_t n);
char* strncpy(char *s, const char *t, int n);
char* strcpy(char* dst_, const char* src_);
char* safestrcpy(char *s, const char *t, int n);
int strlen(const char *s);
int8_t strcmp (const char* a, const char* b);
char* strchr(const char* string, const uint8_t ch);
char* strrchr(const char* string, const uint8_t ch);
char* strcat(char* dst_, const char* src_);
uint32_t strchrs(const char* filename, uint8_t ch);
#endif