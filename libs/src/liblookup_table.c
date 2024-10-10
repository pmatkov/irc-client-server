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

struct LookupPair {
    int key;
    const char *label;
};

struct LookupTable {
    LookupPair *lookupPairs;
    int size;
};

#endif

LookupTable * create_lookup_table(int *keys, const char **labels, int size) {

    if (keys == NULL || labels == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    LookupTable *lookupTable = (LookupTable *) malloc(sizeof(LookupTable));
    if (lookupTable == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    lookupTable->lookupPairs = (LookupPair *) malloc(size * sizeof(LookupPair));
    if (lookupTable->lookupPairs == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    } 

    for (int i = 0; i < size; i++) {

        if (keys[i] < 0 || keys[i] >= size) {
            FAILED("Invalid key", NO_ERRCODE);
        }

        lookupTable->lookupPairs[keys[i]].key = keys[i];
        lookupTable->lookupPairs[i].label = labels[i];

    }

    lookupTable->size = size;
  
    return lookupTable;
}


void delete_lookup_table(LookupTable *lookupTable) {

    if (lookupTable != NULL) {

        free(lookupTable->lookupPairs);
    }

    free(lookupTable);
}

const char * lookup_label(LookupTable *lookupTable, int key) {

    if (lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char *label = NULL;

    if (key >= 0 && key < lookupTable->size) {

        label = lookupTable->lookupPairs[key].label;
    }

    return label;
    
}

void set_label(LookupTable *lookupTable, int key, const char *label) {

    if (lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (key >= 0 && key < lookupTable->size) {

        lookupTable->lookupPairs[key].label = label;
    }    
}

int lookup_key(LookupTable *lookupTable, const char *label) {

    if (lookupTable == NULL || label == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int key = -1;

    for (int i = 0; i < lookupTable->size; i++) {

        if (strcmp(lookupTable->lookupPairs[i].label, label) == 0) {
            key = lookupTable->lookupPairs[i].key;
        }
    }

    return key;
}

LookupPair * get_lookup_pair(LookupTable *lookupTable, int key) {

    if (lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return &lookupTable->lookupPairs[key];
}

int get_pair_key(LookupPair *lookupPair) {

    if (lookupPair == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return lookupPair->key;
}

const char * get_pair_label(LookupPair *lookupPair) {

    if (lookupPair == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return lookupPair->label;
}