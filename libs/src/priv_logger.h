/* --INTERNAL HEADER--
   used for testing */

#ifndef LOGGER_H
#define LOGGER_H

#include "threads.h"
#include "error_control.h"

#include <stdio.h>
#include <pthread.h>

#define LOG(level, msg, ...) \
    log_message(level, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    UNKNOWN_LOGLEVEL,
    LOGLEVEL_COUNT
} LogLevel;

typedef struct {
    FILE *logFile;
    char **logBuffer;
    LogLevel logLevel;
    int logFrequency;
    int stdoutEnabled;
    int logPending;
    int capacity;
    int count;
} Logger;

typedef void (*LogFunc)(void *arg);

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

void write_log_to_file(void);

void log_message(LogLevel level, const char *msg, const char *function, const char *file, int line, ...);
void log_error(ErrorCode errorCode, const char *msg, const char *function, const char *file, int line, int errnosv, ...);

const char * log_level_to_string(LogLevel logLevel);
LogLevel string_to_log_level(const char *string);
int is_valid_log_level(LogLevel logLevel);

int is_stdout_enabled(void);
void enable_stdout_logging(int stdoutEnabled);

void set_log_thread(Thread *thread);
void set_log_thread_callback(NotifyThreadFunc func);
pthread_mutex_t * get_log_mutex(void);
void set_log_pending(int pending);
void notify_log_pending(void);

#ifdef TEST

FILE * open_log_file(char *dirPath, char *identifier);

#endif

#endif