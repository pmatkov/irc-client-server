/* --INTERNAL HEADER--
   used for testing */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "priv_lookup_table.h"

typedef enum {
    INT_TYPE,
    CHAR_TYPE,
    UNKNOWN_DATA_TYPE,
    DATA_TYPE_COUNT
} DataType;

typedef struct {
    DataType dataType;
    Pair *pair;
    union {
        const char *charValue;
        int intValue;
    };
} Property;

typedef struct {
    Property *properties;
    int capacity;
    int count;
} Settings;

Settings * create_settings(int capacity);
void delete_settings(Settings *settings);

void register_property(DataType dataType, Pair *pair, const void *value);

const void * get_property_value(int propertyType);
void set_property_value(int propertyType, const void *value);

DataType get_property_data_type(int propertyType);

int is_property_registered(int propertyType);
int is_valid_property(int propertyType);

void read_settings(LookupTable *lookupTable, const char *fileName);
void write_settings(const char *fileName);

int get_settings_capacity(void);
const char * get_property_label(int propertyType);

#ifdef TEST

void read_property_string(LookupTable *lookupTable, char *buffer);
void create_property_string(char *buffer, int size, Property *property);

#endif

#endif
