#include "time_utils.h"
#include "logger.h"
#include "error.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SEC_TO_MICROSEC (1000 * 1000)

struct Timer {
    struct timeval startTime;
    struct timeval endTime;
    bool active;
};

static void fhmtime(char *timestamp, size_t length, struct tm *timeInfo);
static void fhmstime(char *timestamp, size_t length, struct tm *timeInfo);
static void fdate(char *timestamp, size_t length, struct tm *timeInfo);
static void fdatetime(char *timestamp, size_t length, struct tm *timeInfo);

static void (*format_function[DATETIME_FORMAT_COUNT])(char *, size_t, struct tm *) = {fhmtime, fhmstime, fdate, fdatetime};

void (*get_format_function(DateTimeFormat format))(char *, size_t, struct tm *) {

    if (format >= 0 && format < DATETIME_FORMAT_COUNT) {
        return format_function[format];
    }
    return NULL;
}

void get_datetime(void (*format_function)(char *, size_t, struct tm *), char *timestamp, size_t length) {

    if (format_function == NULL || timestamp == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    time_t timeVal;
    struct tm *timeInfo;

    timeVal = time(NULL);
    timeInfo = localtime(&timeVal);

    format_function(timestamp, length, timeInfo);
}

/* get time in hh:mm format */
static void fhmtime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%H:%M", timeInfo);
}

/* get time in hh:mm:ss format */
static void fhmstime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%H:%M:%S", timeInfo);
}

/* get date in yyyy-mm-dd format */
static void fdate(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%Y-%m-%d", timeInfo);
}

/* get date and time in yyyy-mm-dd hh:mm:ss format */
static void fdatetime(char *timestamp, size_t length, struct tm *timeInfo) {
    strftime(timestamp, length, "%Y-%m-%d %H:%M:%S", timeInfo);
}

Timer * create_timer(void) {

    Timer *timer = (Timer*) malloc(sizeof(Timer));
    if (timer == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    timer->active = 0;

    return timer;
}

void delete_timer(Timer *timer) {

    free(timer);
}

void start_timer(Timer *timer) {

    if (timer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    gettimeofday(&timer->startTime, NULL);
    timer->active = 1;
}

void stop_timer(Timer *timer)  {

    if (timer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    gettimeofday(&timer->endTime, NULL);
}

void reset_timer(Timer *timer) {

    if (timer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    timer->startTime.tv_sec = 0;
    timer->endTime.tv_usec = 0;
    timer->active = 0;
}

long get_elapsed_time(Timer *timer, TimeFormat timeFormat) {

    if (timer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    
    int elapsedTime = 0;

    long seconds = timer->endTime.tv_sec - timer->startTime.tv_sec;

    if (timeFormat == SECONDS) {
        elapsedTime = seconds;
    }
    else if (timeFormat == MICROSECONDS) {
        long microseconds = timer->endTime.tv_usec - timer->startTime.tv_usec;
        elapsedTime = seconds * SEC_TO_MICROSEC + microseconds;
    }

    return elapsedTime;
}

bool is_timer_active(Timer *timer) {

    if (timer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    
    return timer->active;
}

struct itimerval * create_interval_timer(int seconds) {

    struct itimerval *timer = (struct itimerval *) malloc(sizeof(struct itimerval));

    if (timer == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    timer->it_value.tv_sec = seconds;
    timer->it_value.tv_usec = 0;
    timer->it_interval.tv_sec = seconds;
    timer->it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, timer, NULL);

    return timer;
}

void delete_interval_timer(struct itimerval *timer) {

    free(timer);
}