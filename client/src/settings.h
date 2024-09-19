#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum {
    NICKNAME,
    USERNAME,
    REALNAME,
    COLOR,
    UNKNOWN_PROPERTY_TYPE,
    PROPERTY_TYPE_COUNT
} PropertyType;

typedef struct Property Property;
typedef struct Settings Settings;

Settings * create_settings(void);
void delete_settings(Settings *settings);

void set_default_settings(Settings *settings);

char * get_property_value(Settings *settings, PropertyType propertyType);
void set_property_value(Settings *settings, PropertyType propertyType, const char *value);

int is_property_assigned(Settings *settings, PropertyType propertyType);
int get_assigned_properties(Settings *settings);

void read_settings(Settings *settings, const char *fileName);
void write_settings(Settings *settings, const char *fileName);

PropertyType string_to_property_type(const char *string);
const char *property_type_to_string(PropertyType propertyType);

#endif
