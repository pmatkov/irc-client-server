#ifdef TEST
#include "priv_hash_table.h"
#else
#include "hash_table.h"
#endif

#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define DEF_LOAD_FACTOR 1.0
#define MAX_LOAD_FACTOR 2.0

#ifndef TEST

/* each item in the table consists of a key-value pair.
    the value is accessed by computing a hash value from 
    the key */
struct HashItem {
    void *key;
    void *value;
    HashItem *next;
};

/* the table uses comparison and deletion functions 
    for operations on the table. hash value is computed
    using a hash function */
struct HashTable {
    HashItem **items;
    int loadFactor;
    int itemCount;
    int linkCount;
    int capacity;
    HashFunc hashFunc;
    ComparatorFunc comparatorFunc;
    DeleteKeyFunc deleteKeyFunc;
    DeleteValueFunc deleteValueFunc;
};

#endif


STATIC float calculate_load_factor(HashTable *hashTable);
STATIC float get_valid_load_factor(float loadFactor);
STATIC int calculate_table_capacity(int itemCount, float loadFactor);

HashTable* create_hash_table(int maxItems, float loadFactor, HashFunc hashFunc, ComparatorFunc comparatorFunc, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc) {

    int capacity = calculate_table_capacity(maxItems, loadFactor);

    if (capacity <= 0) {
        LOG(NO_ERRCODE, "Invalid capacity");
        return NULL;
    }

    HashTable *hashTable = (HashTable *) malloc(sizeof(HashTable));
    if (hashTable == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    hashTable->items = (HashItem **) malloc(capacity * sizeof(HashItem*));
    if (hashTable->items == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < capacity; i++) {
        hashTable->items[i] = NULL;
    }

    loadFactor = get_valid_load_factor(loadFactor);

    hashTable->loadFactor = loadFactor;
    hashTable->itemCount = 0;
    hashTable->linkCount = 0;
    hashTable->capacity = capacity;
    hashTable->hashFunc = hashFunc;
    hashTable->comparatorFunc = comparatorFunc;
    hashTable->deleteKeyFunc = deleteKeyFunc;
    hashTable->deleteValueFunc = deleteValueFunc;

    return hashTable; 
}

void delete_hash_table(HashTable *hashTable) {

    if (hashTable != NULL) {

        for (int i = 0; i < hashTable->capacity; i++) {

            HashItem *current = hashTable->items[i];

            while (current != NULL) {
                HashItem *next = current->next; 

                delete_hash_item(current, hashTable->deleteKeyFunc, hashTable->deleteValueFunc);
                current = next; 
            }

        }
        free(hashTable->items);
    }
    free(hashTable);
}

HashItem * create_hash_item(void *key, void *value) {

    if (key == NULL || value == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    HashItem *item = (HashItem *) malloc(sizeof(HashItem));
    if (item == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    item->key = key;
    item->value = value;
    item->next = NULL;

    return item;
}

void delete_hash_item(HashItem *item, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc) {

    if (item != NULL) {

        if (deleteKeyFunc != NULL) {

            deleteKeyFunc(item->key);
        } 
        if (deleteValueFunc != NULL) {

            deleteValueFunc(item->value);
        }
    }

    free(item);
}

bool is_hash_table_empty(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    } 

    return hashTable->itemCount == 0;
}

bool is_hash_table_full(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    } 

    return calculate_load_factor(hashTable) >= hashTable->loadFactor;
}

int insert_item_to_hash_table(HashTable *hashTable, HashItem *item) {

    if (hashTable == NULL || item == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int inserted = 0;

    if (!is_hash_table_full(hashTable) && find_item_in_hash_table(hashTable, item->key) == NULL) {

        unsigned long index = hashTable->hashFunc(item->key) % hashTable->capacity;

        if (hashTable->items[index] == NULL) {

            hashTable->items[index] = item;
            hashTable->itemCount++;
        }
        else {
            HashItem *current = hashTable->items[index];
            HashItem *previous = NULL; 

            while (current != NULL) {

                previous = current;
                current = current->next;
            }
            
            previous->next = item;
            hashTable->linkCount++;
        }
        inserted = 1;
    }

    return inserted;
}

int remove_item_from_hash_table(HashTable *hashTable, void *key) {

    if (hashTable == NULL || key == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int removed = 0;

    if (!is_hash_table_empty(hashTable)) {

        unsigned long index = hashTable->hashFunc(key) % hashTable->capacity;

        HashItem *current = hashTable->items[index];
        HashItem *previous = NULL;

        while (current != NULL && !hashTable->comparatorFunc(current->key, key)) {

            previous = current;
            current = current->next;
        }

        if (current != NULL) {

            if (previous == NULL) {
                    
                    hashTable->items[index] = current->next;

                    if (hashTable->items[index] == NULL) {
                        hashTable->itemCount--;
                    }
                    else {
                        hashTable->linkCount--;
                    }
            }
            else {
                previous->next = current->next;
                hashTable->linkCount--;
            }
            delete_hash_item(current, hashTable->deleteKeyFunc, hashTable->deleteValueFunc);
            current = NULL;
            removed = 1;
        }
    }
    
    return removed;
}

HashItem * find_item_in_hash_table(HashTable *hashTable, void *key) {

    if (hashTable == NULL || key == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    HashItem *foundItem = NULL;

    if (!is_hash_table_empty(hashTable)) {

        unsigned long index = hashTable->hashFunc(key) % hashTable->capacity;
        HashItem *current = hashTable->items[index];

        while (current != NULL && foundItem == NULL) {

            if (hashTable->comparatorFunc(current->key, key)) {
                foundItem = current;
            }
            else {
                current = current->next;
            }
        }
    }
    return foundItem;
}

void * get_key(HashItem *item) {

    void *key = NULL;

    if (item != NULL) {
        key = item->key;
    }
    return key;
}

void * get_value(HashItem *item) {

    void *value = NULL;

    if (item != NULL) {
        value = item->value;
    }
    return value;
}

int get_total_items(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    } 

    return hashTable->itemCount + hashTable->linkCount;
}

unsigned long djb2_hash(void *key) {

    unsigned long hash = 5381;
    int c;

    while ((c = *(char *)key++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

unsigned long fnv1a_hash(void *key) {

    const unsigned char *byte_ptr = (unsigned char *)key;
    unsigned long hash = 2166136261U;

    for (int i = 0; i < sizeof(int); i++) {
        hash ^= byte_ptr[i];
        hash *= 16777619U;
    }

    return hash;
}

bool are_strings_equal(void *key1, void *key2) {

    return strcmp((char*)key1, (char*)key2) == 0;
}

bool are_ints_equal(void *key1, void *key2) {

    return *(int*)key1 == *(int*)key2;
}

/* a load factor represents the ratio of available 
    buckets to elements in the table, including those 
    chained during conflicts. when MAX_LOAD_FACTOR 
    is reached, the table is considered full and should 
    be resized */
STATIC float calculate_load_factor(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    } 

    float loadFactor = 0.0;
    
    if (hashTable->capacity) {

      loadFactor = (hashTable->itemCount + hashTable->linkCount) / (float) hashTable->capacity;
    }

    return loadFactor;
}


STATIC float get_valid_load_factor(float loadFactor) {

    if (loadFactor <= 0) {
        loadFactor = DEF_LOAD_FACTOR;
    }

    if (loadFactor > MAX_LOAD_FACTOR) {
        loadFactor = MAX_LOAD_FACTOR;
    }

    return loadFactor;
}

STATIC int calculate_table_capacity(int itemCount, float loadFactor) {

    int capacity = 0;

    loadFactor = get_valid_load_factor(loadFactor);

    if (fmod(itemCount, loadFactor) == 0.0) {
        capacity = itemCount / loadFactor;
    }
    else {
        capacity = itemCount / loadFactor + 1;
    }

    return capacity;
}
