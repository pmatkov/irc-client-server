#ifndef SETTINGS_H
#define SETTINGS_H

#include "lookup_table.h"

/* dataType represents the data types used
    for properties */
typedef enum {
    INT_TYPE,
    CHAR_TYPE,
    UNKNOWN_DATA_TYPE,
    DATA_TYPE_COUNT
} DataType;

/* in this context, property represents a 
    configuration option for the app. each
    property has an initial value that can
    be modified according to the user's preferences.
    a group of properties represents the settings.  */
typedef struct Property Property;

/* settings represent a collection of configurable
    properties */
typedef struct Settings Settings;

Settings * create_settings(int capacity);
void delete_settings(Settings *settings);

/* before it can be used, a property has to be
    registered */
void register_property(DataType dataType, Pair *pair, const void *value);

/* accessor and mutator methods for property
    values */
const void * get_property_value(int propertyType);
void set_property_value(int propertyType, const void *value);

DataType get_property_data_type(int propertyType);

int is_property_registered(int propertyType);
int is_valid_property(int propertyType);

int get_settings_capacity(void);
const char * get_property_label(int propertyType);

/* reads settings from a file */
void read_settings(LookupTable *lookupTable, const char *fileName);

/* write settings from memory
    to the file */
void write_settings(const char *fileName);

#endif
