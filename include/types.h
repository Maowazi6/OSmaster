#ifndef _TYPES_H_
#define _TYPES_H_


#define TRUE 1
#define FALSE 0
#define true 1
#define false 0
#define NULL ((void*)0)
#define UNUSED __attribute__ ((unused))
typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long long int int64_t;
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;
typedef char bool;
typedef char* va_list;
typedef unsigned int size_t;//机器位宽

#endif 
