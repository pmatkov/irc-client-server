#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum {
    NICKNAME,
    USERNAME,
    REALNAME,
    HOSTNAME,
    PORT,
    MAX_CLIENTS,
    UNKNOWN_PROPERTY_TYPE,
    PROPERTY_TYPE_COUNT
} PropertyType;

typedef enum {
    CLIENT_PROPERTY,
    SERVER_PROPERTY,
    COMMON_PROPERTY,
    UNKNOWN_PROPERTY_USER,
    PROPERTY_USER_COUNT
} PropertyUser;

typedef struct Property Property;
typedef struct Settings Settings;

void set_default_settings(void);

const char * get_property_value(PropertyType propertyType);
void set_property_value(PropertyType propertyType, const char *value);

int is_property_assigned(PropertyType propertyType);
int get_assigned_properties_count(PropertyUser propertyUser);

void read_settings(const char *fileName, PropertyUser propertyUser);
void write_settings(const char *fileName, PropertyUser propertyUser);

PropertyType string_to_property_type(const char *string);
const char * property_type_to_string(PropertyType propertyType);

PropertyUser get_property_user(PropertyType propertyType);

#endif
