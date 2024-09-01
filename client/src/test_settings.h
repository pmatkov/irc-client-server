#ifndef SETTINGS_H
#define SETTINGS_H

#define MAX_KEYVAL 64

typedef enum {
    NICKNAME,
    USERNAME,
    HOSTNAME,
    REALNAME,
    COLOR,
    UNKNOWN_PROPERTY_TYPE,
    PROPERTY_TYPE_COUNT
} PropertyType;

typedef struct {
    PropertyType propertyType;
    char value[MAX_KEYVAL + 1];
} Property;

typedef struct {
    Property *properties;
    int allocatedSize;
} Settings;

Settings * create_settings(void);
void delete_settings(Settings *settings);

void set_default_settings(Settings *settings);

const char *get_property_type_string(PropertyType propertyType);

Property * get_property(Settings *settings, PropertyType propertyType);
void set_property(Settings *settings, PropertyType propertyType, const char *value);

char * get_property_value(Settings *settings, PropertyType propertyType);
int is_property_assigned(Settings *settings, PropertyType propertyType);
int get_assigned_properties(Settings *settings);

void read_settings(Settings *settings, const char *fileName);
void write_settings(Settings *settings, const char *fileName);

#ifdef TEST

PropertyType string_to_property_type(const char *string);
int is_valid_property_type(PropertyType propertyType);

#endif

#endif
