#ifdef TEST
#include "priv_settings.h"
#else
#include "settings.h"
#include "common.h"
#endif

#include "enum_utils.h"
#include "path.h"
#include "error_control.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#define DEF_SETTINGS_DIR "/data/"
#define DEF_SETTINGS_FILE "settings.conf"

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_PATH_LEN 64

#ifndef TEST

/* an option is identified by its data type, an
    optionType, a label and a value. an optionType
    should be an enum that starts at 0 and ends
    at capacity - 1, ensuring that each option is
    accessibly by its index */
struct Option {
    DataType dataType;
    int optionType;
    const char *label;
    union {
        char charValue[MAX_CHARS + 1];
        int intValue;
    };
};

struct Settings {
    Option *options;
    int capacity;
    int count;
};

#endif

STATIC int label_to_option_type(const char *label);
STATIC void read_option_string(char *buffer);
STATIC void create_option_string(char *buffer, int size, Option *option);

static Settings *settings = NULL;

Settings * create_settings(int capacity) {

    if (capacity <= 0) {
        LOG(NO_ERRCODE, "Invalid capacity");
        return NULL;
    }

    settings = (Settings *) malloc(sizeof(Settings));
    if (settings == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    } 

    settings->options = (Option *) malloc(capacity * sizeof(Option));
    if (settings->options  == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < capacity; i++) {
        settings->options[i].dataType = UNKNOWN_DATA_TYPE;
        settings->options[i].optionType = i;
        settings->options[i].label = NULL;
        memset(settings->options[i].charValue, '\0', sizeof(settings->options[i].charValue));
    }
    settings->capacity = capacity;
    settings->count = 0;
    
    return settings;
}

void delete_settings(Settings *settings) {

    if (settings != NULL) {

        free(settings->options);
    }
    free(settings);    
}

void register_option(DataType dataType, int optionType, const char *label, void *value) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (is_valid_option_type(optionType)) {

        settings->options[optionType].dataType = dataType;
        settings->options[optionType].optionType = optionType;
        settings->options[optionType].label = label;

        if (dataType == CHAR_TYPE) {
            safe_copy(settings->options[optionType].charValue, MAX_CHARS + 1, (char*)value);
        }
        else if (dataType == INT_TYPE) {
            settings->options[optionType].intValue = *((int*)value);
        }

        settings->count++;
    }
}

void unregister_option(int optionType) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (is_valid_option_type(optionType)) {

        reset_option(optionType);
        settings->count--;
    }
}

void reset_option(int optionType) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (is_valid_option_type(optionType)) {

        settings->options[optionType].dataType = UNKNOWN_DATA_TYPE;
        settings->options[optionType].optionType = optionType;
        settings->options[optionType].label = NULL;
        memset(settings->options[optionType].charValue, '\0', sizeof(settings->options[optionType].charValue));
    }
}

void * get_option_value(int optionType) {

    void *value = NULL;

    if (settings != NULL && is_valid_option_type(optionType)) {

        if (settings->options[optionType].dataType == CHAR_TYPE) {
            value = settings->options[optionType].charValue;
        }
        else if (settings->options[optionType].dataType == INT_TYPE) {
            value = &settings->options[optionType].intValue;
        }
    }
    return value;
}

void set_option_value(int optionType, void *value) {

    if (settings == NULL || value == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* a option value can be set only if the
        option has been previously registered */
    if (is_valid_option_type(optionType) && is_option_registered(optionType)) {

        if (settings->options[optionType].dataType == CHAR_TYPE) {
            safe_copy(settings->options[optionType].charValue, MAX_CHARS + 1, (char*)value);
        }
        else if (settings->options[optionType].dataType == INT_TYPE) {

            settings->options[optionType].intValue = *((int*)value);
        }
    }
}

int get_int_option_value(int optionType) {

    void *value = get_option_value(optionType);

    return value != NULL ? *(int*)value : 0;
}

const char * get_char_option_value(int optionType) {

    return get_option_value(optionType);
}


DataType get_option_data_type(int optionType) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    DataType dataType = UNKNOWN_DATA_TYPE;

    if (is_valid_option_type(optionType)) {

        dataType = settings->options[optionType].dataType;
    }

    return dataType;
}

const char * get_option_label(int optionType) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *label = NULL;

    if (is_valid_option_type(optionType)) {

        label = settings->options[optionType].label;
    }

    return label;
}

bool is_option_registered(int optionType) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    bool registered = 0;

    if (is_valid_option_type(optionType)) {

        registered = settings->options[optionType].dataType != UNKNOWN_DATA_TYPE;
    }

    return registered;

}

bool is_valid_option_type(int optionType) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return optionType >= 0 && optionType < settings->capacity;
}

int get_settings_capacity(void) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return settings->capacity;
}

void read_settings(const char *fileName) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char path[MAX_PATH_LEN + 1] = {'\0'};

    /* if a fileName is not provided, a default fileName
        will be used */
    if (fileName == NULL) {

        char relativePath[MAX_PATH_LEN + 1] = DEF_SETTINGS_DIR;

        strcat(relativePath, DEF_SETTINGS_FILE);

        if (!create_path(path, MAX_PATH_LEN, relativePath)) {
            FAILED(NO_ERRCODE, "Error creating path");
        }

        fileName = path;
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        return;
    }

    char buffer[MAX_CHARS + 1];

    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        read_option_string(buffer);
    }
    fclose(file);
    
}

void write_settings(const char *fileName) {

    if (settings == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char path[MAX_PATH_LEN + 1] = {'\0'};

    /* if a fileName is not provided, a default dir and 
        fileName will be used */
    if (fileName == NULL) {

        if (!create_path(path, MAX_PATH_LEN, DEF_SETTINGS_DIR)) {
            FAILED(NO_ERRCODE, "Error creating path");
        }

        if (!is_dir(path)) {
            create_dir(path);
        }

        strncat(path, DEF_SETTINGS_FILE, MAX_PATH_LEN - strlen(path));

        fileName = path;
    }

    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        FAILED(NO_ERRCODE, "Error opening file");
    }

    char buffer[MAX_CHARS + 1] = {'\0'};

    for (int i = 0; i < settings->capacity; i++) {

        if (is_option_registered(i)) {

            create_option_string(buffer, ARRAY_SIZE(buffer), &settings->options[i]);

            fprintf(file, buffer);
            fprintf(file, "\n");
        }
    }
    fclose(file);
}

STATIC int label_to_option_type(const char *label) {

    if (settings == NULL || label == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int optionType = -1;

    for (int i = 0; i < settings->capacity; i++) {

        if (settings->options[i].label != NULL && strcmp(settings->options[i].label, label) == 0) {
                optionType = settings->options[i].optionType;
                break;
            }
    }

    return optionType;
}

/* parse the option string and set the option 
    value */
STATIC void read_option_string(char *buffer) {

    if (settings == NULL || buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    char *saveptr = NULL;
    char *label = strtok_r(buffer, "=", &saveptr);
    char *value = strtok_r(NULL, "\n", &saveptr);

    if (label != NULL && value != NULL) {

        int optionType = label_to_option_type(label);
        DataType dataType = get_option_data_type(optionType);

        if (dataType == INT_TYPE) {

            int intValue = str_to_uint(value);
            set_option_value(optionType, &intValue);
        }
        else if (dataType == CHAR_TYPE) {
            set_option_value(optionType, value);
        } 
    }
}

/* create the option string from the current
    value */
STATIC void create_option_string(char *buffer, int size, Option *option) {

    if (settings == NULL || buffer == NULL || option == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char *value = NULL;
    char numStr[MAX_CHARS + 1] = {'\0'};
    const int ASSIGN_LEN = 1;

    if (option->dataType == INT_TYPE) {

        uint_to_str(numStr, ARRAY_SIZE(numStr), option->intValue);
        value = numStr;
    }
    else if (option->dataType == CHAR_TYPE) {

        value = option->charValue;
    }

    if (value != NULL && size > strlen(option->label) + strlen(value) + ASSIGN_LEN) {

        memset(buffer, '\0', size);
        strcat(buffer, option->label);
        strcat(buffer, "=");
        strcat(buffer, value);
    }
}