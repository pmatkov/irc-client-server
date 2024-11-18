/* --INTERNAL HEADER--
   used for testing */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define MAX_LOAD_FACTOR 2.0

typedef unsigned long (*HashFunc)(void *key);
typedef int (*ComparatorFunc)(void *key1, void *key2);
typedef void (*DeleteKeyFunc)(void *key);
typedef void (*DeleteValueFunc)(void *value);

typedef struct HashItem {
    void *key;
    void *value;
    struct HashItem *next;
} HashItem;

typedef struct HashTable {
    HashItem **items;
    int capacity;
    int itemsCount;
    int linksCount;
    HashFunc hashFunc;
    ComparatorFunc comparatorFunc;
    DeleteKeyFunc deleteKeyFunc;
    DeleteValueFunc deleteValueFunc;
} HashTable;

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

#ifdef TEST

int is_hash_table_empty(HashTable *hashTable);
int is_hash_table_full(HashTable *hashTable);

float calculate_load_factor(HashTable *hashTable);

#endif

#endif