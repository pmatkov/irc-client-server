#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

typedef struct {
    int key;
    const char *value;
} Pair;

typedef struct {
    Pair *pairs;
    int size;
} LookupTable;

LookupTable * create_lookup_table(int *keys, const char **values, int size);
void delete_lookup_table(LookupTable *lookupTable);

const char * lookup_value(LookupTable *lookupTable, int key);
void set_value(LookupTable *lookupTable, int key, const char *value);

int lookup_key(LookupTable *lookupTable, const char *value);

Pair * get_lookup_pair(LookupTable *lookupTable, int key);

int get_pair_key(Pair *pair);
const char * get_pair_value(Pair *pair);

#endif