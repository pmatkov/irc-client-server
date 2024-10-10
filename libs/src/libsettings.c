#ifdef TEST
#include "priv_settings.h"
#else
#include "settings.h"
#endif

#include "path.h"
#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

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

#define MAX_PATH_LEN 64

#ifndef TEST

struct Property {
    DataType dataType;
    LookupPair *lookupPair;
    union {
        const char *charValue;
        int intValue;
    };
};

struct Settings {
    Property *properties;
    int capacity;
    int count;
};

#endif

STATIC void read_property_string(Settings *settings, LookupTable *lookupTable, char *buffer);
STATIC void create_property_string(char *buffer, int size, Property *property);

static Settings *settings = NULL;

Settings * create_settings(int capacity) {

    if (settings == NULL) {

        settings = (Settings *) malloc(sizeof(Settings));
        if (settings == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        } 

        settings->properties = (Property *) malloc(capacity * sizeof(Property));
        if (settings->properties  == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        }

        for (int i = 0; i < capacity; i++) {

            settings->properties[i].dataType = UNKNOWN_DATA_TYPE;
            settings->properties[i].lookupPair = NULL;
            settings->properties[i].charValue = NULL;
        }

        settings->capacity = capacity;
    }
    return settings;
}

void delete_settings(Settings *settings) {

    if (settings != NULL) {

        free(settings->properties);
    }
    free(settings);    
}

void register_property(Settings *settings, DataType dataType, LookupPair *lookupPair, void *value) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int propertyType = get_pair_key(lookupPair);

    if (is_valid_property(settings, propertyType)) {

        settings->properties[propertyType].dataType = dataType;
        settings->properties[propertyType].lookupPair = lookupPair;

        if (dataType == CHAR_TYPE) {
            settings->properties[propertyType].charValue = value;
        }
        else if (dataType == INT_TYPE) {
            settings->properties[propertyType].intValue = *((int*)value);
        }
    }
}

void * get_property_value(Settings *settings, int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    void *value = NULL;

    if (is_valid_property(settings, propertyType)) {

        if (settings->properties[propertyType].dataType == CHAR_TYPE) {
            value = (char*) settings->properties[propertyType].charValue;
        }
        else if (settings->properties[propertyType].dataType == INT_TYPE) {
            value = &settings->properties[propertyType].intValue;
        }
    }
    return value;
}

void set_property_value(Settings *settings, int propertyType, void *value) {

    if (settings == NULL || value == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_valid_property(settings, propertyType) && is_property_registered(settings, propertyType)) {

        if (settings->properties[propertyType].dataType == CHAR_TYPE) {

            settings->properties[propertyType].charValue = value;
        }
        else if (settings->properties[propertyType].dataType == INT_TYPE) {

            settings->properties[propertyType].intValue = *((int*)value);
        }
    }
}

DataType get_property_data_type(Settings *settings, int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    DataType dataType = UNKNOWN_DATA_TYPE;

    if (is_valid_property(settings, propertyType)) {

        dataType = settings->properties[propertyType].dataType;
    }

    return dataType;
}

int is_property_registered(Settings *settings, int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return settings->properties[propertyType].dataType != UNKNOWN_DATA_TYPE;

}

int is_valid_property(Settings *settings, int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return propertyType >= 0 && propertyType < settings->capacity;

}

void read_settings(Settings *settings, LookupTable *lookupTable, const char *fileName) {

    if (settings == NULL || lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char path[MAX_PATH_LEN + 1] = {'\0'};

    if (fileName == NULL) {

        if (!create_path(path, MAX_PATH_LEN, "/data/buzz.conf")) {
            FAILED("Error creating path", NO_ERRCODE);
        }

        fileName = path;
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        return;
    }

    char buffer[MAX_CHARS + 1];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        read_property_string(settings, lookupTable, buffer);
    }
    fclose(file);
    
}

void write_settings(Settings *settings, const char *fileName) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char path[MAX_PATH_LEN + 1] = {'\0'};

    if (fileName == NULL) {

        if (!create_path(path, MAX_PATH_LEN, "/data/")) {
            FAILED("Error creating path", NO_ERRCODE);
        }

        if (!is_dir(path)) {
            create_dir(path);
        }

        strncat(path, "buzz.conf", MAX_PATH_LEN - strlen(path));

        fileName = path;
    }

    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    char buffer[MAX_CHARS + 1];

    for (int i = 0; i < settings->capacity; i++) {

        if (is_property_registered(settings, i)) {

            create_property_string(buffer, MAX_CHARS + 1, &settings->properties[i]);

            fprintf(file, buffer);
            fprintf(file, "\n");
        }
    }
    fclose(file);
}

STATIC void read_property_string(Settings *settings, LookupTable *lookupTable, char *buffer) {

    char *label = strtok(buffer, "=");
    char *value = strtok(NULL, "\n");

    if (label != NULL && value != NULL) {

        int propertyType = lookup_key(lookupTable, label);
        DataType dataType = get_property_data_type(settings, propertyType);

        if (dataType == INT_TYPE) {

            int intValue = str_to_uint(value);
            set_property_value(settings, propertyType, &intValue);
        }
        else if (dataType == CHAR_TYPE) {
            set_property_value(settings, propertyType, (char *) value);
        } 
    }
}

STATIC void create_property_string(char *buffer, int size, Property *property) {

    memset(buffer, '\0', size);

    strncat(buffer, get_pair_label(property->lookupPair), size);
    strncat(buffer, "=", size - strlen(buffer));

    if (property->dataType == INT_TYPE) {

        char numStr[MAX_CHARS + 1] = {'\0'};

        uint_to_str(buffer, MAX_CHARS + 1, property->intValue);
        strncat(buffer, numStr, size - strlen(buffer));
    }
    else if (property->dataType == CHAR_TYPE) {

        strncat(buffer, property->charValue, size - strlen(buffer));
    }

}