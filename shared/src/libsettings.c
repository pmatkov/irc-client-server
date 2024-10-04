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

#define MAX_LABEL_LEN 64
#define MAX_VALUE_LEN 64
#define MAX_PATH_LEN 64

#ifndef TEST

struct Property {
    PropertyType propertyType;
    PropertyUser propertyUser;
    char *label;
    char value[MAX_VALUE_LEN + 1];
};

struct Settings {
    Property *properties;
    int capacity;
};

#endif

static Property PROPERTIES[] = {
    {NICKNAME, CLIENT_PROPERTY, "nickname", ""},
    {USERNAME, CLIENT_PROPERTY, "username", ""},
    {REALNAME, CLIENT_PROPERTY, "realname", ""},
    {COLOR, CLIENT_PROPERTY, "color", ""},
    {HOSTNAME, SERVER_PROPERTY, "hostname", ""},
    {PORT, SERVER_PROPERTY, "port", ""},
    {MAX_CLIENTS, SERVER_PROPERTY, "max_clients", ""},
    {UNKNOWN_PROPERTY_TYPE, COMMON_PROPERTY, "unknown", ""},
};

_Static_assert(sizeof(PROPERTIES) / sizeof(PROPERTIES[0]) == PROPERTY_TYPE_COUNT, "Array size mismatch");


STATIC int is_valid_property_type(PropertyType propertyType);

void set_default_settings(void) {

    struct passwd *userRecord = getpwuid(getuid());

    if (userRecord != NULL) {
        set_property_value(NICKNAME, userRecord->pw_name);
        set_property_value(USERNAME, userRecord->pw_name);
    }

    set_property_value(REALNAME, "anonymous");
    set_property_value(COLOR, "1");
    set_property_value(HOSTNAME, "irc.example.com");
    set_property_value(PORT, "50100");
    set_property_value(MAX_CLIENTS, "1024");
}

const char * get_property_value(PropertyType propertyType) {

    char * value = NULL;

    if (is_valid_property_type(propertyType)) {
        value = PROPERTIES[propertyType].value;
    }

    return value;
}

void set_property_value(PropertyType propertyType, const char *value) {

    if (value == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_valid_property_type(propertyType)) {

        safe_copy(PROPERTIES[propertyType].value, MAX_VALUE_LEN + 1, value);
    }
}

int is_property_assigned(PropertyType propertyType) {

    int assigned = 0;

    if (is_valid_property_type(propertyType)) {
        assigned = strcmp(PROPERTIES[propertyType].value, "") != 0;
    }
    return assigned;
}

int get_assigned_properties_count(PropertyUser propertyUser) {

    int assigned = 0;

    for (int i = 0; i < PROPERTY_TYPE_COUNT; i++) {

        if (PROPERTIES[i].propertyUser == propertyUser && strcmp(PROPERTIES[i].value, "") != 0) {
            assigned++;
        }
    }
    return assigned;
}

void read_settings(const char *fileName, PropertyUser propertyUser) {

    char path[MAX_PATH_LEN + 1] = {'\0'};

    if (fileName == NULL) {

        if (!set_default_path(path, MAX_PATH_LEN, "/data/")) {
            FAILED("Error creating path", NO_ERRCODE);
        }

        strncat(path, "buzz.conf", MAX_PATH_LEN - strlen(path));

        fileName = path;
    }

    FILE *file = fopen(fileName, "r");

    if (file != NULL) {

        char buffer[MAX_VALUE_LEN * 2 + 1];

        while (fgets(buffer, sizeof(buffer), file) != NULL) {

            char *key = strtok(buffer, "=");
            char *value = strtok(NULL, "\n");

            if (key != NULL && value != NULL) {

                PropertyType propertyType = string_to_property_type(key);

                if (is_valid_property_type(propertyType) && PROPERTIES[propertyType].propertyUser == propertyUser) {
                    set_property_value(string_to_property_type(key), value);
                }
            }
        }
        fclose(file);
    }
}

void write_settings(const char *fileName, PropertyUser propertyUser) {

    char path[MAX_PATH_LEN + 1] = {'\0'};

    if (fileName == NULL) {

        if (!set_default_path(path, MAX_PATH_LEN, "/data/")) {
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

    // extra char for '='
    char buffer[MAX_LABEL_LEN + MAX_VALUE_LEN + 2];

    for (int i = 0; i < PROPERTY_TYPE_COUNT; i++) {

        if (PROPERTIES[i].propertyUser == propertyUser && is_property_assigned(PROPERTIES[i].propertyType)) {

            memset(buffer, '\0', sizeof(buffer));

            strcat(buffer, PROPERTIES[i].label);
            strcat(buffer, "=");
            strcat(buffer, PROPERTIES[i].value);
            
            fprintf(file, buffer);
            fprintf(file, "\n");
        }
    }
    fclose(file);
}

PropertyType string_to_property_type(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    PropertyType propertyType = UNKNOWN_PROPERTY_TYPE; 

    for (int i = 0; i < PROPERTY_TYPE_COUNT; i++) {

        if (strncmp(PROPERTIES[i].label, string, MAX_LABEL_LEN) == 0) {
            propertyType = (PropertyType) i;
        }
    }
    return propertyType;
}

const char * property_type_to_string(PropertyType propertyType) {

    const char *string = PROPERTIES[UNKNOWN_PROPERTY_TYPE].label;

    if (is_valid_property_type(propertyType)) {
        string = PROPERTIES[propertyType].label;
    }

    return string;
}

STATIC int is_valid_property_type(PropertyType propertyType) {

    return propertyType >= 0 && propertyType < PROPERTY_TYPE_COUNT;
}

PropertyUser get_property_user(PropertyType propertyType) {

    PropertyUser propertyUser = UNKNOWN_PROPERTY_USER;
    
    if (is_valid_property_type(propertyType)) {
        propertyUser = PROPERTIES[propertyType].propertyUser;
    }

    return propertyUser;
}
