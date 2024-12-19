#ifndef ENUM_UTILS_H
#define ENUM_UTILS_H

#include <stdbool.h>

bool is_valid_enum_type(int enumVal, int enumCount);

const char * enum_type_to_string(int enumVal, int enumCount, const char **labels);
int string_to_enum_type(const char **labels, int enumCount, const char *string);

#endif