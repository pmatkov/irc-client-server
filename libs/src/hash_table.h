#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>

typedef unsigned long (*HashFunc)(void *key);
typedef bool (*ComparatorFunc)(void *key1, void *key2);
typedef void (*DeleteKeyFunc)(void *key);
typedef void (*DeleteValueFunc)(void *value);

/* a generic hash table that can may any data type */
typedef struct HashItem HashItem;
typedef struct HashTable HashTable;

HashTable *create_hash_table(int maxItems, float loadFactor, HashFunc hashFunc, ComparatorFunc comparatorFunc, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc);
void delete_hash_table(HashTable *hashTable);

HashItem * create_hash_item(void *key, void *value);
void delete_hash_item(HashItem *item, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc);

bool is_hash_table_empty(HashTable *hashTable);
bool is_hash_table_full(HashTable *hashTable);

int insert_item_to_hash_table(HashTable *table, HashItem *item);
int remove_item_from_hash_table(HashTable *hashTable, void *key);
HashItem * find_item_in_hash_table(HashTable *table, void *key);

void * get_key(HashItem *item);
void * get_value(HashItem *item);
int get_total_items(HashTable *hashTable);

unsigned long djb2_hash(void *key);
unsigned long fnv1a_hash(void *key);

bool are_strings_equal(void *key1, void *key2);
bool are_ints_equal(void *key1, void *key2);

#endif