#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

typedef struct {
    int key;
    const char *label;
} LookupPair;

typedef struct {
    LookupPair *lookupPairs;
    int size;
} LookupTable;

LookupTable * create_lookup_table(int *keys, const char **labels, int size);
void delete_lookup_table(LookupTable *lookupTable);

const char * lookup_label(LookupTable *lookupTable, int key);
void set_label(LookupTable *lookupTable, int key, const char *label);

int lookup_key(LookupTable *lookupTable, const char *label);

LookupPair * get_lookup_pair(LookupTable *lookupTable, int key);

int get_pair_key(LookupPair *lookupPair);
const char * get_pair_label(LookupPair *lookupPair);

#endif