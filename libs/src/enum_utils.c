#include "enum_utils.h"
#include "string_utils.h"

#include <stddef.h>
#include <string.h>

bool is_valid_enum_type(int enumType, int enumCount) {

    return enumType >= 0 && enumType < enumCount;
}

const char * enum_type_to_string(int enumVal, int enumCount, const char **labels) {

    const char *string = NULL;

    if (is_valid_enum_type(enumVal, enumCount) && labels != NULL) {
        string = labels[enumVal];
    };

    return string;
}

int string_to_enum_type(const char **labels, int enumCount, const char *string) {

    int enumVal = enumCount - 1;

    if (labels != NULL && string != NULL) {

        for (int i = 0; i < enumCount; i++) {

            if (labels[i] != NULL && strcmp(labels[i], string) == 0) {
                enumVal = i;
                break;
            }
        };
    }
    return enumVal;
}