#ifndef _GENERAL_H
#define _GENERAL_H

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))
#define offset(struct_type,member) (int)(&((struct_type*)0)->member)
#define STRUCT_ENTRY(struct_type, struct_member_name, member_ptr) \
	 (struct_type*)((int)member_ptr - offset(struct_type, struct_member_name))



#endif