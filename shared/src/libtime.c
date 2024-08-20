#include "time.h"
#include "logger.h"
#include "error.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

static void ftime(char *timestamp, size_t length, struct tm *timeInfo);
static void fdate(char *timestamp, size_t length, struct tm *timeInfo);
static void fdatetime(char *timestamp, size_t length, struct tm *timeInfo);

static void (*format_function[DATETIME_COUNT])(char *, size_t, struct tm *) = {ftime, fdate, fdatetime};

void (*get_format_function(DateTimeFormat format))(char *, size_t, struct tm *) {

    if (format >= 0 && format < DATETIME_COUNT) {
        return format_function[format];
    }
    return NULL;
}

void get_datetime(void (*format_function)(char *, size_t, struct tm *), char *timestamp, size_t length) {

    if (format_function == NULL || timestamp == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    time_t timeVal;
    struct tm *timeInfo;

    timeVal = time(NULL);
    timeInfo = localtime(&timeVal);

    format_function(timestamp, length, timeInfo);
}

static void ftime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%H:%M:%S", timeInfo);
}

static void fdate(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%Y-%m-%d", timeInfo);
}

static void fdatetime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%Y-%m-%d %H:%M:%S", timeInfo);
}
