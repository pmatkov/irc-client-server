/* --INTERNAL HEADER--
   used for testing */
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#define HM_TIME_LENGTH 6
#define HMS_TIME_LENGTH 9
#define DATE_LENGTH 11
#define DATETIME_LENGTH 21

#define DEF_TIMER_INTERVAL 60

typedef enum {
    HM_TIME,
    HMS_TIME,
    DATE,
    DATETIME,
    DATETIME_FORMAT_COUNT
} DateTimeFormat;

typedef enum {
    SECONDS,
    MICROSECONDS
} TimeFormat;

typedef struct {
    struct timeval startTime;
    struct timeval endTime;
    int active;
} Timer;


void (*get_format_function(DateTimeFormat format))(char *, size_t, struct tm *);
void get_datetime(void (*format_function)(char *, size_t, struct tm *), char *timestamp, size_t length);

Timer * create_timer(void);
void delete_timer(Timer *timer);
void start_timer(Timer *timer);
void stop_timer(Timer *timer);
void reset_timer(Timer *timer);
long get_elapsed_time(Timer *timer, TimeFormat timeFormat);
bool is_timer_active(Timer *timer);

struct itimerval * create_interval_timer(int seconds);
void delete_interval_timer(struct itimerval *timer);

#endif