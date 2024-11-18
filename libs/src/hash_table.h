#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef unsigned long (*HashFunc)(void *key);
typedef int (*ComparatorFunc)(void *key1, void *key2);
typedef void (*DeleteKeyFunc)(void *key);
typedef void (*DeleteValueFunc)(void *value);

/* generic hash table that can use any data type */
typedef struct HashItem HashItem;
typedef struct HashTable HashTable;

HashTable* create_hash_table(int capacity, HashFunc hashFunc, ComparatorFunc comparatorFunc, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc);
void delete_hash_table(HashTable *hashTable);

HashItem * create_hash_item(void *key, void *value);
void delete_hash_item(HashItem *item, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc);

int insert_item_to_hash_table(HashTable *table, HashItem *item);
int remove_item_from_hash_table(HashTable *hashTable, void *key);
HashItem * find_item_in_hash_table(HashTable *table, void *key);

unsigned long string_hash_function(void *key);
int are_strings_equal(void *key1, void *key2);

void * get_key(HashItem *item);
void * get_value(HashItem *item);

#endif