#ifndef DATA_TYPE_H
#define DATA_TYPE_H

#include "common.h"

typedef enum {
    INT_TYPE,
    CHAR_TYPE,
    FLOAT_TYPE,
    VOID_TYPE,
    UNKNOWN_DATA_TYPE,
    DATA_TYPE_COUNT
} DataType;

typedef union {
    int itemInt;
    char itemChar[MAX_CHARS + 1];
    float itemFloat;
    void *itemVoid;
} DataItem;

#endif