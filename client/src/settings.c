#ifdef TEST
#include "test_settings.h"
#else
#include "settings.h"
#endif


#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_VALUE 64
#define MAX_CHARS 512

#ifndef TEST

struct Property {
    PropertyType propertyType;
    char value[MAX_VALUE + 1];
};

struct Settings {
    Property *properties;
    int allocatedSize;
};

#endif

STATIC PropertyType string_to_property_type(const char *string);
STATIC int is_valid_property_type(PropertyType propertyType);

static const char *PROPERTY_TYPE_STRING[] = {
    "nickname",
    "username",
    "hostname",
    "realname",
    "color",
    "unknown property type"
};

_Static_assert(sizeof(PROPERTY_TYPE_STRING) / sizeof(PROPERTY_TYPE_STRING[0]) == PROPERTY_TYPE_COUNT, "Array size mismatch");

Settings * create_settings(void) {

    Settings *settings = (Settings *) malloc(sizeof(Settings));
    if (settings == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    settings->properties = (Property *) malloc((PROPERTY_TYPE_COUNT - 1) * sizeof(Property));
    if (settings->properties == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    for (int i = 0; i < PROPERTY_TYPE_COUNT - 1; i++) {
        settings->properties[i].propertyType = (PropertyType) i;
    }

    settings->allocatedSize = PROPERTY_TYPE_COUNT - 1;

    return settings;
}

void delete_settings(Settings *settings) {

    if (settings != NULL) {

        free(settings->properties);
    }
    free(settings);
}

void set_default_settings(Settings *settings) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct passwd *userRecord = getpwuid(getuid());

    if (userRecord != NULL) {
        set_property(settings, NICKNAME, userRecord->pw_name);
        set_property(settings, USERNAME, userRecord->pw_name);
    }

    char hostname[MAX_CHARS + 1];

    if (gethostname(hostname, sizeof(hostname)) == 0) {
       set_property(settings, HOSTNAME, hostname);
    }

    set_property(settings, REALNAME, "");
    set_property(settings, COLOR, "1");
}

const char *get_property_type_string(PropertyType propertyType) {

    return PROPERTY_TYPE_STRING[propertyType];
}

Property * get_property(Settings *settings, PropertyType propertyType) {

    Property *property = NULL;

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_valid_property_type(propertyType)) {
        property = &settings->properties[propertyType];
    }
    return property;
}

void set_property(Settings *settings, PropertyType propertyType, const char *value) {

    if (settings == NULL || value == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_valid_property_type(propertyType) && strnlen(value, MAX_VALUE + 1) != MAX_VALUE + 1) {

        strcpy(settings->properties[propertyType].value, value);
    }
}

char * get_property_value(Settings *settings, PropertyType propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char * value = NULL;

    if (is_valid_property_type(propertyType)) {
        value = settings->properties[propertyType].value;
    }

    return value;
}

int is_property_assigned(Settings *settings, PropertyType propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int assigned = 0;

    if (is_valid_property_type(propertyType)) {
        assigned = strcmp(settings->properties[propertyType].value, "") != 0;
    }
    return assigned;
}

int get_assigned_properties(Settings *settings) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int assigned = 0;

    for (int i = 0; i < settings->allocatedSize; i++) {

        if (strcmp(settings->properties[i].value, "") != 0) {
            assigned++;
        }
    }
    return assigned;
}

void read_settings(Settings *settings, const char *fileName) {

    if (settings == NULL || fileName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    char buffer[MAX_VALUE * 2 + 1];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        char *key = strtok(buffer, "=");
        char *value = strtok(NULL, "\n");

        set_property(settings, string_to_property_type(key), value);
    }
    fclose(file);
}

void write_settings(Settings *settings, const char *fileName) {

    if (settings == NULL || fileName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    // extra char for '='
    char buffer[MAX_VALUE * 2 + 2];

    for (int i = 0; i < settings->allocatedSize; i++) {

        if (is_property_assigned(settings, settings->properties[i].propertyType) && settings->properties[i].propertyType != UNKNOWN_PROPERTY_TYPE) {

            memset(buffer, '\0', MAX_VALUE * 2 + 2);

            strcat(buffer, PROPERTY_TYPE_STRING[settings->properties[i].propertyType]);
            strcat(buffer, "=");
            strcat(buffer, settings->properties[i].value);
            
            fprintf(file, buffer);
            fprintf(file, "\n");
        }
    }
    fclose(file);
}

STATIC PropertyType string_to_property_type(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    PropertyType propertyType = UNKNOWN_PROPERTY_TYPE; 

    for (int i = 0; i < PROPERTY_TYPE_COUNT - 1; i++) {

        if (strncmp(PROPERTY_TYPE_STRING[i], string, MAX_VALUE) == 0) {
            propertyType = (PropertyType) i;
        }
    }
    return propertyType;
}

STATIC int is_valid_property_type(PropertyType propertyType) {

    return propertyType >= 0 && propertyType < PROPERTY_TYPE_COUNT - 1;
}
