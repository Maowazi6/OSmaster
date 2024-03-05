#ifndef _HASH_BUCKET_H
#define _HASH_BUCKET_H
#include "types.h"
#include "assert.h"

typedef int KeyType;
typedef int ValueType;

typedef struct HashNode
{
	KeyType key;
	ValueType value;
	struct HashNode* next;
}HashNode;

typedef struct HashBucket
{
	HashNode ** tables;
	uint32_t len;
	uint32_t size;
}HashTable;

HashNode *BuyHashNode(KeyType k, ValueType v);
void HTBInit(HashTable *htb, uint32_t len);
void HTBDestory(HashTable* htb);
int HTBInsert(HashTable *htb, KeyType key, ValueType value);
int HTBRemove(HashTable *htb, KeyType key);
HashNode* HTBFind(HashTable *htb, KeyType key);
int HTBSize(HashTable* htb);
bool HTBEmpty(HashTable* htb);
int str_hash_insert(HashTable *htb, char *key, ValueType value);
int str_hash_remove(HashTable *htb, char *key);
HashNode* str_hash_find(HashTable *htb, char *key);
//void Print(HashTable* htb);
//void Test();


#endif