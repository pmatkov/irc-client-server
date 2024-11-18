/* --INTERNAL HEADER--
   used for testing */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "data_type.h"
#include "string_utils.h"

typedef struct {
    DataType dataType;
    int optionType;
    const char *label;
    union {
        char charValue[MAX_CHARS + 1];
        int intValue;
    };
} Option;

typedef struct {
    Option *options;
    int capacity;
    int count;
} Settings;

Settings * create_settings(int capacity);
void delete_settings(Settings *settings);

void register_option(DataType dataType, int optionType, const char *label, void *value);
void unregister_option(int optionType);

void reset_option(int optionType);

void * get_option_value(int optionType);
void set_option_value(int optionType, void *value);

int get_int_option_value(int optionType);
char * get_char_option_value(int optionType);

DataType get_option_data_type(int optionType);
const char * get_option_label(int optionType);

int is_option_registered(int optionType);
int is_valid_option_type(int optionType);

void read_settings(const char *fileName);
void write_settings(const char *fileName);

int get_settings_capacity(void);

#ifdef TEST

int label_to_option_type(const char *label);
void read_option_string(char *buffer);
void create_option_string(char *buffer, int size, Option *option);

#endif

#endif
