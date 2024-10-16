#ifdef TEST
#include "priv_lookup_table.h"
#else
#include "lookup_table.h"
#endif

#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#ifndef TEST

struct Pair {
    int key;
    const char *value;
};

struct LookupTable {
    Pair *pairs;
    int size;
};

#endif

LookupTable * create_lookup_table(int *keys, const char **values, int size) {

    if (keys == NULL || values == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    LookupTable *lookupTable = (LookupTable *) malloc(sizeof(LookupTable));
    if (lookupTable == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    lookupTable->pairs = (Pair *) malloc(size * sizeof(Pair));
    if (lookupTable->pairs == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    } 

    for (int i = 0; i < size; i++) {

        if (keys[i] >= 0 && keys[i] < size) {
            lookupTable->pairs[keys[i]].key = keys[i];
            lookupTable->pairs[i].value = values[i];
        }
    }

    lookupTable->size = size;
  
    return lookupTable;
}


void delete_lookup_table(LookupTable *lookupTable) {

    if (lookupTable != NULL) {

        free(lookupTable->pairs);
    }

    free(lookupTable);
}

const char * lookup_value(LookupTable *lookupTable, int key) {

    if (lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char *value = NULL;

    if (key >= 0 && key < lookupTable->size) {

        value = lookupTable->pairs[key].value;
    }

    return value;
    
}

void set_value(LookupTable *lookupTable, int key, const char *value) {

    if (lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (key >= 0 && key < lookupTable->size) {

        lookupTable->pairs[key].value = value;
    }    
}

int lookup_key(LookupTable *lookupTable, const char *value) {

    if (lookupTable == NULL || value == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int key = -1;

    for (int i = 0; i < lookupTable->size; i++) {

        if (strcmp(lookupTable->pairs[i].value, value) == 0) {
            key = lookupTable->pairs[i].key;
        }
    }

    return key;
}

Pair * get_lookup_pair(LookupTable *lookupTable, int key) {

    if (lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Pair *pair = NULL;

    if (key >= 0 && key < lookupTable->size) {

        pair = &lookupTable->pairs[key];
    }

    return pair;
}

int get_pair_key(Pair *pair) {

    if (pair == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return pair->key;
}

const char * get_pair_value(Pair *pair) {

    if (pair == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return pair->value;
}