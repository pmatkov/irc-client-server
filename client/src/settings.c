#include "settings.h"
#include "../../shared/src/errorctrl.h"
#include "../../shared/src/logger.h"

#include <stdio.h>

STATIC SettingType string_to_setting_type(const char *settingType);
STATIC int is_valid_setting(SettingType settingType);

static const char *SETTING_TYPE_STRING[] = {
    "nickname",
    "username",
    "realname",
    "color",
    "unknown setting"
};

_Static_assert(sizeof(SETTING_TYPE_STRING) / sizeof(SETTING_TYPE_STRING[0]) == SETTING_TYPE_COUNT, "Array size mismatch");

static SettingsCollection *settingsCollection = NULL;

SettingsCollection * create_settings_collection(void) {

    settingsCollection = (SettingsCollection *) malloc(sizeof(SettingsCollection));
    if (settingsCollection == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    settingsCollection->settings = (Setting *) malloc(SETTING_TYPE_COUNT * sizeof(Setting));
    if (settingsCollection == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    for (int i = 0; i < SETTING_TYPE_COUNT; i++) {
        settingsCollection->settings[i].settingType = UNKNOWN_SETTING_TYPE;
    }

    settingsCollection->allocatedSize = SETTING_TYPE_COUNT;

    return settingsCollection;
}

void delete_settings_collection(SettingsCollection *settingsCollection) {

    if (settingsCollection != NULL) {
        free(settingsCollection->settings);
    }
    free(settingsCollection);
}

STATIC SettingType string_to_setting_type(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    for (int i = 0; i < SETTING_TYPE_COUNT; i++) {

        if (strncmp(string, SETTING_TYPE_STRING[i], MAX_KEYVAL) == 0) {
            return (SettingType) i;
        }
    }
    return UNKNOWN_SETTING_TYPE;
}

STATIC int is_valid_setting(SettingType settingType) {

    return settingType >= 0 && settingType < SETTING_TYPE_COUNT;
}

const char * get_setting_string(SettingType settingType) {

    if (is_valid_setting(settingType)) {
        return SETTING_TYPE_STRING[settingType];
    }
    return NULL;
}

SettingsCollection * get_all_settings(void) {

    if (settingsCollection == NULL) {
        FAILED("Settings collection not created", NO_ERRCODE);
    }

    return settingsCollection;
}

Setting * get_setting(SettingType settingType) {

    if (settingsCollection == NULL) {
        FAILED("Settings collection not created", NO_ERRCODE);
    }

    if (is_valid_setting(settingType)) {
        return &settingsCollection->settings[settingType];
    }
    return NULL;
}

void set_setting(const char *key, const char *value) {

    if (key == NULL || value == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    if (settingsCollection == NULL) {
        FAILED("Settings collection not created", NO_ERRCODE);
    }
    SettingType settingType;
    if ((settingType = string_to_setting_type(key)) != UNKNOWN_SETTING_TYPE && \
         strnlen(key, MAX_KEYVAL + 1) != MAX_KEYVAL + 1 && strnlen(value, MAX_KEYVAL + 1) != MAX_KEYVAL + 1) {

        settingsCollection->settings[settingType].settingType = value == "" ? UNKNOWN_SETTING_TYPE : settingType;
        strcpy(settingsCollection->settings[settingType].key, key);
        strcpy(settingsCollection->settings[settingType].value, value);
    }
}

void read_settings(const char *fileName) {

    if (fileName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (settingsCollection == NULL) {
        FAILED("Settings collection not created", NO_ERRCODE);
    }

    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    char lnBuffer[MAX_KEYVAL * 2 + 1];

    while (fgets(lnBuffer, sizeof(lnBuffer), file) != NULL) {

        char *key = strtok(lnBuffer, "=");
        char *value = strtok(NULL, "\n");

        if (key != NULL && value != NULL) {

            set_setting(key, value);
        }
    }
    fclose(file);
}

void write_settings(const char *fileName) {

    if (fileName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (settingsCollection == NULL) {
        FAILED("Settings collection not created", NO_ERRCODE);
    }

    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    // added extra char for '='
    char lnBuffer[MAX_KEYVAL * 2 + 2];

    for (int i = 0; i < settingsCollection->allocatedSize; i++) {

        if (settingsCollection->settings[i].settingType != UNKNOWN_SETTING_TYPE)

        strcat(lnBuffer, settingsCollection->settings[i].key);
        strcat(lnBuffer, "=");
        strcat(lnBuffer, settingsCollection->settings[i].value);
        fprintf(file, lnBuffer);
        fprintf(file, "\n");

    }

    fclose(file);
}
