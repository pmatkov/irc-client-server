#ifndef TIME_H
#define TIME_H

#include <stddef.h>
#include <time.h>

#define TIME_LENGTH 9
#define DATE_LENGTH 11
#define DATETIME_LENGTH 21

typedef enum {
    TIME,
    DATE,
    DATETIME,
    DATETIME_COUNT
} DateTimeFormat;

void (*get_format_function(DateTimeFormat format))(char *, size_t, struct tm *);
void get_datetime(void (*format_function)(char *, size_t, struct tm *), char *timestamp, size_t length);

#endif