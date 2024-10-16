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

#define DEFAULT_SETTINGS_DIR "/data/"
#define DEFAULT_SETTINGS_FILE "settings.conf"

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_PATH_LEN 64

#ifndef TEST

/* a property is identified by its data type,
    a lookup pair (enum key and string translation)
    and a value */
struct Property {
    DataType dataType;
    Pair *pair;
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

STATIC void read_property_string(LookupTable *lookupTable, char *buffer);
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
            settings->properties[i].pair = NULL;
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

void register_property(DataType dataType, Pair *pair, const void *value) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int propertyType = get_pair_key(pair);

    if (is_valid_property(propertyType)) {

        settings->properties[propertyType].dataType = dataType;
        settings->properties[propertyType].pair = pair;

        if (dataType == CHAR_TYPE) {
            settings->properties[propertyType].charValue = value;
        }
        else if (dataType == INT_TYPE) {
            settings->properties[propertyType].intValue = *((const int*)value);
        }
    }
}

const void * get_property_value(int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    const void *value = NULL;

    if (is_valid_property(propertyType)) {

        if (settings->properties[propertyType].dataType == CHAR_TYPE) {
            value = settings->properties[propertyType].charValue;
        }
        else if (settings->properties[propertyType].dataType == INT_TYPE) {
            value = &settings->properties[propertyType].intValue;
        }
    }
    return value;
}

void set_property_value(int propertyType, const void *value) {

    if (settings == NULL || value == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_valid_property(propertyType) && is_property_registered(propertyType)) {

        if (settings->properties[propertyType].dataType == CHAR_TYPE) {

            settings->properties[propertyType].charValue = value;
        }
        else if (settings->properties[propertyType].dataType == INT_TYPE) {

            settings->properties[propertyType].intValue = *((const int*)value);
        }
    }
}

DataType get_property_data_type(int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    DataType dataType = UNKNOWN_DATA_TYPE;

    if (is_valid_property(propertyType)) {

        dataType = settings->properties[propertyType].dataType;
    }

    return dataType;
}

int is_property_registered(int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return settings->properties[propertyType].dataType != UNKNOWN_DATA_TYPE;

}

int is_valid_property(int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return propertyType >= 0 && propertyType < settings->capacity;
}

int get_settings_capacity(void) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return settings->capacity;
}

const char * get_property_label(int propertyType) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return get_pair_value(settings->properties[propertyType].pair);
}

void read_settings(LookupTable *lookupTable, const char *fileName) {

    if (settings == NULL || lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char path[MAX_PATH_LEN + 1] = {'\0'};

    /* if a fileName is NULL a default fileName will
        be used */
    if (fileName == NULL) {

        char relativePath[MAX_PATH_LEN + 1] = DEFAULT_SETTINGS_DIR;

        strcat(relativePath, DEFAULT_SETTINGS_FILE);

        if (!create_path(path, MAX_PATH_LEN, relativePath)) {
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

        read_property_string(lookupTable, buffer);
    }
    fclose(file);
    
}

void write_settings(const char *fileName) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char path[MAX_PATH_LEN + 1] = {'\0'};

    /* if a fileName is NULL a default dir and fileName
        will be used */

    if (fileName == NULL) {

        if (!create_path(path, MAX_PATH_LEN, DEFAULT_SETTINGS_DIR)) {
            FAILED("Error creating path", NO_ERRCODE);
        }

        if (!is_dir(path)) {
            create_dir(path);
        }

        strncat(path, DEFAULT_SETTINGS_FILE, MAX_PATH_LEN - strlen(path));

        fileName = path;
    }

    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    char buffer[MAX_CHARS + 1];

    for (int i = 0; i < settings->capacity; i++) {

        if (is_property_registered(i)) {

            create_property_string(buffer, MAX_CHARS + 1, &settings->properties[i]);

            fprintf(file, buffer);
            fprintf(file, "\n");
        }
    }
    fclose(file);
}

/* parse the property string and save
    the property to memory */
STATIC void read_property_string(LookupTable *lookupTable, char *buffer) {

    char *label = strtok(buffer, "=");
    char *value = strtok(NULL, "\n");

    if (label != NULL && value != NULL) {

        int propertyType = lookup_key(lookupTable, label);
        DataType dataType = get_property_data_type(propertyType);

        if (dataType == INT_TYPE) {

            int intValue = str_to_uint(value);
            set_property_value(propertyType, &intValue);
        }
        else if (dataType == CHAR_TYPE) {
            set_property_value(propertyType, (char *) value);
        } 
    }
}

/* create the property string from the current
    value in memory  */
STATIC void create_property_string(char *buffer, int size, Property *property) {

    memset(buffer, '\0', size);

    strncat(buffer, get_pair_value(property->pair), size);
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