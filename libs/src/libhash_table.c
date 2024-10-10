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

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

#define LOAD_FACTOR 2.0

struct HashItem {
    void *key;
    void *value;
    HashItem *next;
};

struct HashTable {
    HashItem **items;
    int capacity;
    int itemsCount;
    int linksCount;
    HashFunc hashFunc;
    ComparatorFunc comparatorFunc;
    DeleteKeyFunc deleteKeyFunc;
    DeleteValueFunc deleteValueFunc;
};

#endif

STATIC int is_hash_table_empty(HashTable *hashTable);
STATIC int is_hash_table_full(HashTable *hashTable);
STATIC float calculate_load_factor(HashTable *hashTable);

HashTable* create_hash_table(int capacity, HashFunc hashFunc, ComparatorFunc comparatorFunc, DeleteKeyFunc deleteKeyFunc, DeleteValueFunc deleteValueFunc) {

    HashTable *hashTable = (HashTable *) malloc(sizeof(HashTable));
    if (hashTable == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    hashTable->items = (HashItem **) malloc(capacity * sizeof(HashItem*));
    if (hashTable->items == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    for (int i = 0; i < capacity; i++) {
        hashTable->items[i] = NULL;
    }

    hashTable->capacity = capacity;
    hashTable->itemsCount = 0;
    hashTable->linksCount = 0;
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
        FAILED(NULL, ARG_ERROR);
    }

    HashItem *item = (HashItem *) malloc(sizeof(HashItem));
    if (item == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
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

STATIC int is_hash_table_empty(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    } 

    return hashTable->itemsCount == 0;
}

STATIC int is_hash_table_full(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    } 

    return calculate_load_factor(hashTable) >= LOAD_FACTOR;
}

int insert_item_to_hash_table(HashTable *hashTable, HashItem *item) {

    if (hashTable == NULL || item == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int inserted = 0;

    if (!is_hash_table_full(hashTable)) {

        unsigned long index = hashTable->hashFunc(item->key) % hashTable->capacity;

        if (hashTable->items[index] == NULL) {

            hashTable->items[index] = item;
            hashTable->itemsCount++;
        }
        else {
            HashItem *current = hashTable->items[index];
            HashItem *previous = NULL; 

            while (current != NULL) {

                previous = current;
                current = current->next;
            }
            
            previous->next = item;
            hashTable->linksCount++;
        }
        inserted = 1;
    }

    return inserted;
}

int remove_item_from_hash_table(HashTable *hashTable, void *key) {

    if (hashTable == NULL || key == NULL) {
        FAILED(NULL, ARG_ERROR);
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
                        hashTable->itemsCount--;
                    }
                    else {
                        hashTable->linksCount--;
                    }
            }
            else {
                previous->next = current->next;
                hashTable->linksCount--;
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
        FAILED(NULL, ARG_ERROR);
    }

    HashItem *current = NULL;

    if (!is_hash_table_empty(hashTable)) {

        unsigned long index = hashTable->hashFunc(key) % hashTable->capacity;
        current = hashTable->items[index];

        while (current != NULL && !hashTable->comparatorFunc(current->key, key)) {

            current = current->next;
        }
    }
    return current;
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

unsigned long string_hash_function(void *key) {

    unsigned long hash = 5381;
    int c;

    while ((c = *(char *)key++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

int are_strings_equal(void *key1, void *key2) {

    return strcmp((char*)key1, (char*)key2) == 0;
}

STATIC float calculate_load_factor(HashTable *hashTable) {

    if (hashTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    } 

    float loadFactor = 0.0;
    
    if (hashTable->capacity) {

      loadFactor = (hashTable->itemsCount + hashTable->linksCount) / (float) hashTable->capacity;
    }

    return loadFactor;
}
