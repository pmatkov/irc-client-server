/* --INTERNAL HEADER--
   used for unit testing */

#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stddef.h>
#include <time.h>

#define HM_TIME_LENGTH 6
#define HMS_TIME_LENGTH 9
#define DATE_LENGTH 11
#define DATETIME_LENGTH 21

typedef enum {
    HM_TIME,
    HMS_TIME,
    DATE,
    DATETIME,
    DATETIME_COUNT
} DateTimeFormat;

typedef struct {
    time_t startTime;
    time_t endTime;
    int isActive;
} Timer;

void (*get_format_function(DateTimeFormat format))(char *, size_t, struct tm *);
void get_datetime(void (*format_function)(char *, size_t, struct tm *), char *timestamp, size_t length);

Timer * create_timer(void);
void delete_timer(Timer *timer);
void start_timer(Timer *timer);
void stop_timer(Timer *timer);
void reset_timer(Timer *timer);
int get_elapsed_time(Timer *timer);
int is_timer_active(Timer *timer);

#endif