/* --INTERNAL HEADER--
   used for unit testing */

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
    LookupPair *lookupPair;
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

void register_property(Settings *settings, DataType dataType, LookupPair *lookupPair, void *value);

void * get_property_value(Settings *settings, int propertyType);
void set_property_value(Settings *settings, int propertyType, void *value);

DataType get_property_data_type(Settings *settings, int propertyType);

int is_property_registered(Settings *settings, int propertyType);
int is_valid_property(Settings *settings, int propertyType);

void read_settings(Settings *settings, LookupTable *lookupTable, const char *fileName);
void write_settings(Settings *settings, const char *fileName);

#ifdef TEST

void read_property_string(Settings *settings, LookupTable *lookupTable, char *buffer);
void create_property_string(char *buffer, int size, Property *property);

#endif

#endif
