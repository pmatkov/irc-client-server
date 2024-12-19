#ifndef SETTINGS_H
#define SETTINGS_H

#include "data_type.h"

#include <stdbool.h>

/* an option is a configurable item within the settings */
typedef struct Option Option;

/* settings represent a collection of configurable
    options */
typedef struct Settings Settings;

Settings * create_settings(int capacity);
void delete_settings(Settings *settings);

/* properties are registered by assigning a data type, 
    a label and a corresponding value */
void register_option(DataType dataType, int , const char *label, void *value);

/* option may be unregistered be specifying an option
    type */
void unregister_option(int optionType);

/* set default option value */
void reset_option(int optionType);

void * get_option_value(int optionType);
void set_option_value(int optionType, void *value);

int get_int_option_value(int optionType);
const char * get_char_option_value(int optionType);

DataType get_option_data_type(int optionType);
const char * get_option_label(int optionType);

bool is_option_registered(int optionType);
bool is_valid_option_type(int optionType);

int get_settings_capacity(void);

/* read settings from the file */
void read_settings(const char *fileName);

/* write settings to the file */
void write_settings(const char *fileName);

#endif
