#ifndef SETTINGS_H
#define SETTINGS_H

#define MAX_KEYVAL 64

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

typedef enum {
    NICKNAME,
    USERNAME,
    REALNAME,
    COLOR,
    UNKNOWN_SETTING_TYPE,
    SETTING_TYPE_COUNT
} SettingType;

typedef struct {
    SettingType settingType;
    char key[MAX_KEYVAL + 1];
    char value[MAX_KEYVAL + 1];
} Setting;

typedef struct {
    Setting *settings;
    int allocatedSize;
} SettingsCollection;

SettingsCollection * create_settings_collection(void);
void delete_settings_collection(SettingsCollection *settingsCollection);

SettingsCollection * get_all_settings(void);
Setting * get_setting(SettingType settingType);
void set_setting(const char *key, const char *value);

void read_settings(const char *fileName);
void write_settings(const char *fileName);

#ifdef TEST

STATIC SettingType string_to_setting_type(const char *settingType);
STATIC int is_valid_setting(SettingType settingType);

#endif

#endif
