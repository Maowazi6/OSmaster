#include "Hash_Bucket.h"
#include "assert.h"


unsigned int BKDRHash(char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;
 
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
 
    return (hash & 0x7FFFFFFF);
}


uint32_t HTBHashFunc(KeyType key, uint32_t len)
{
	return key % len;
}

HashNode *BuyHashNode(KeyType k, ValueType v)
{
	HashNode *node = (HashNode*)malloc(sizeof(HashNode));
	node->key = k;
	node->value = v;
	node->next = NULL;
	return node;
}

void HTBInit(HashTable* htb, uint32_t len)
{
	ASSERT(htb);
	htb->len = len;
	htb->size = 0;
	htb->tables = (HashNode **)malloc(sizeof(HashNode*)*htb->len);
	memset(htb->tables, 0, sizeof(HashNode*)*htb->len);
}

void HTBDestory(HashTable* htb)
{
	ASSERT(htb);
	uint32_t i = 0;
	for (i = 0; i < htb->len; i++)
	{
		HashNode* cur = htb->tables[i];
		while (cur)
		{
			HashNode* next = cur->next;
			free(cur);
			cur = next;
		}
		htb->tables[i] = NULL;
	}
	htb->tables = NULL;
}


void HTBCheckCapacity(HashTable* htb)
{
	ASSERT(htb);
	if (htb->size == htb->len)
	{
		uint32_t i = 0;
		HashTable newhtb;
		newhtb.len = 2*htb->len;
		HTBInit(&newhtb, newhtb.len);
		for (i = 0; i < htb->len; i++)
		{
			HashNode* cur = htb->tables[i];
			while (cur)
			{
				HashNode* next = cur->next;
				uint32_t index = HTBHashFunc(cur->key, newhtb.len);

				cur->next = newhtb.tables[index];
				newhtb.tables[index] = cur;
				cur = next;
			}
			htb->tables[i] = NULL;
		}
		HTBDestory(htb);
		htb->tables = newhtb.tables;
		htb->len = newhtb.len;
	}
}

int HTBInsert(HashTable *htb, KeyType key,ValueType value)
{
	ASSERT(htb);
	HTBCheckCapacity(htb);
	uint32_t index = 0;
	HashNode *cur;
	HashNode *newNode;
	index = HTBHashFunc(key, htb->len);
	cur = htb->tables[index];
	while (cur)
	{
		if (cur->key == key)
		{	
			printf("This key:%d repeat", key);
			return -1;
		}
		cur = cur->next;
	}

	newNode = BuyHashNode(key, value);
	newNode->next = htb->tables[index];
	htb->tables[index] = newNode;
	htb->size++;
	return 0;
}

int HTBRemove(HashTable *htb, KeyType key)
{
	ASSERT(htb);
	uint32_t index;
	HashNode* cur = NULL;
	HashNode* prev = NULL;

	index = HTBHashFunc(key, htb->len);
	cur = htb->tables[index];
	while (cur)
	{
		if (cur->key == key)
		{
			if (prev == NULL)
			{
				htb->tables[index] = cur->next;
			}
			else
			{
				prev->next = cur->next;
			}
			free(cur);
			htb->size--;
			return 0;
		}
		prev = cur;
		cur = cur->next;
	}
	return -1;
}

HashNode* HTBFind(HashTable *htb, KeyType key)
{
	ASSERT(htb);
	int index = 0;
	HashNode *cur;
	index = HTBHashFunc(key, htb->len);
	cur = htb->tables[index];
	while (cur)
	{
		if (cur->key == key)
		{
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}



int HTBSize(HashTable* htb)
{
	ASSERT(htb);
	return htb->size;
}

bool HTBEmpty(HashTable* htb)
{
	ASSERT(htb);
	if (htb->size == 0)
	{
		return true;
	}
	return false;
}


int str_hash_insert(HashTable *htb, char *key, ValueType value){
	unsigned int key1 = BKDRHash(key);
	if(HTBInsert(htb, key1, value)==-1){
		return -1;
	};
}

int str_hash_remove(HashTable *htb, char *key){
	unsigned int key1 = BKDRHash(key);
	if(HTBRemove(htb, key1)==-1){
		printf("No such key : %s\n", key);
	}
}

HashNode* str_hash_find(HashTable *htb, char *key){
	unsigned int key1 = BKDRHash(key);
	return HTBFind(htb, key1);
}




//void Print(HashTable* htb)
//{
//	uint32_t i = 0;
//	ASSERT(htb);
//	for (i = 0; i < htb->len; i++)
//	{
//		HashNode* cur = htb->tables[i];
//		printf("table[%d]->", i);
//		while (cur)
//		{
//			printf("[%d:%d]->", cur->key, cur->value);
//			cur = cur->next;
//		}
//		printf("\n");
//	}
//	printf("\n");
//}

//void* pprinnt(uint32_t key, char **argv);
//void* build_rm(uint32_t key, char **argv);
//void Test()
//{
//	HashTable htb;
//	HTBInit(&htb, 50);
//	int i = 0;
//
//	for (i = 0; i < 20; i++)
//	{
//		HTBInsert(&htb, i, 0);
//	}
//	HTBInsert(&htb, 60, 0);
//	HTBInsert(&htb, 10, 0);
//	printf("%d\n", HTBFind(&htb, 10)->key);
//	Print(&htb);
//	printf("%d\n", HTBSize(&htb));
//	
//	HTBDestory(&htb);
//
//}

//void* pprinnt(uint32_t key, char **argv){
//	printf("key : %d\n",key);
//}
//void* build_rm(uint32_t key, char **argv){
//	printf("key1 : %d\n",key);
//}
//
//void str_test(){
//	HashTable htb;
//	HTBInit(&htb, 50);
//	str_hash_insert(&htb,"ls",pprinnt);
//	str_hash_insert(&htb,"rm",build_rm);
//	HashNode *node = str_hash_find(&htb,"ls");
//	shell_func pfunc = node->value;
//	pfunc(20,NULL);
//	node = str_hash_find(&htb,"rm");
//	pfunc = node->value;
//	pfunc(20,NULL);
//	str_hash_remove(&htb,"rm");
//	str_hash_remove(&htb,"rm");
//}