#include "time_utils.h"
#include "logger.h"
#include "error.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

struct Timer {
    time_t startTime;
    time_t endTime;
};

static void fhmtime(char *timestamp, size_t length, struct tm *timeInfo);
static void fhmstime(char *timestamp, size_t length, struct tm *timeInfo);
static void fdate(char *timestamp, size_t length, struct tm *timeInfo);
static void fdatetime(char *timestamp, size_t length, struct tm *timeInfo);

static void (*format_function[DATETIME_COUNT])(char *, size_t, struct tm *) = {fhmtime, fhmstime, fdate, fdatetime};

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

static void fhmtime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%H:%M", timeInfo);
}

static void fhmstime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%H:%M:%S", timeInfo);
}

static void fdate(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%Y-%m-%d", timeInfo);
}

static void fdatetime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%Y-%m-%d %H:%M:%S", timeInfo);
}


Timer * create_timer(void) {

    Timer *timer = (Timer*) malloc(sizeof(Timer));
    if (timer == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    timer->startTime = 0;
    timer->endTime = 0;

    return timer;
}

void delete_timer(Timer *timer) {

    free(timer);
}


void start_timer(Timer *timer) {

    if (timer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    timer->startTime = time(NULL);
}

void stop_timer(Timer *timer)  {

    if (timer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    timer->endTime = time(NULL);
}

void reset_timer(Timer *timer) {

    if (timer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    timer->startTime = 0;
    timer->endTime = 0;
}

int get_elapsed_time(Timer *timer) {

    if (timer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return timer->endTime - timer->startTime;
}