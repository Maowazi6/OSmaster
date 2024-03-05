#include "list.h"
#include "types.h"

void list_init (struct list* list) {
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

//把链表元素elem插入在元素before之前
void list_insert_before(struct list_elem* before, struct list_elem* elem) { 
	bool old_status = set_intr(false);
	before->prev->next = elem; 
	
	elem->prev = before->prev;
	elem->next = before;
	
	before->prev = elem;
	
	set_intr(old_status);
}

//头插法
void list_push(struct list* plist, struct list_elem* elem) {
	list_insert_before(plist->head.next, elem); // 在队头插入elem
}

//尾插法
void list_append(struct list* plist, struct list_elem* elem) {
	list_insert_before(&plist->tail, elem);    
}


void list_remove(struct list_elem* pelem) {
	bool old_status = set_intr(false);
	pelem->prev->next = pelem->next;
	pelem->next->prev = pelem->prev;
	
	set_intr(old_status);
}


struct list_elem* list_pop(struct list* plist) {
	struct list_elem* elem = plist->head.next;
	list_remove(elem);
	return elem;
} 


bool elem_find(struct list* plist, struct list_elem* obj_elem) {
	struct list_elem* elem = plist->head.next;
	while (elem != &plist->tail) {
		if (elem == obj_elem) {
		return true;
		}
		elem = elem->next;
	}
	return false;
}

//func是判断第一个符合条件的元素
struct list_elem* list_traversal(struct list* plist, function func, int arg) {
	struct list_elem* elem = plist->head.next;
	if (list_empty(plist)) { 
		return NULL;
	}
	
	while (elem != &plist->tail) {
		if(func(elem, arg)) {		  
			return elem;
		}					  
		elem = elem->next;	       
	}
	return NULL;
}


uint32_t list_len(struct list* plist) {
	struct list_elem* elem = plist->head.next;
	uint32_t length = 0;
	while (elem != &plist->tail) {
		length++; 
		elem = elem->next;
	}
	return length;
}


bool list_empty(struct list* plist) {		
	return (plist->head.next == &plist->tail ? true : false);
}
