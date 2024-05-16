#ifndef _GENERAL_H
#define _GENERAL_H

#include "types.h"

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))

#define offset(struct_type,member) (int)(&((struct_type*)0)->member)

#define STRUCT_ENTRY(struct_type, struct_member_name, member_ptr) \
	 (struct_type*)((int)member_ptr - offset(struct_type, struct_member_name))
	 
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                   \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );})


#endif