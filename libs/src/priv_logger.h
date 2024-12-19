/* --INTERNAL HEADER--
   used for testing */
#ifndef LOGGER_H
#define LOGGER_H

#include "threads.h"
#include "error_control.h"

#include <stdio.h>
#include <stdbool.h>
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
    bool stdoutEnabled;
    bool logPending;
    int currentCount;
    int totalCount;
    int capacity;
} Logger;

typedef void (*LogFunc)(void *arg);

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

void write_log_to_file(void);

void log_message(LogLevel level, const char *msg, const char *function, const char *file, int line, ...);
void log_error(ErrorCode errorCode, const char *msg, const char *function, const char *file, int line, int errnosv, ...);

const char ** get_log_level_strings(void);

bool is_stdout_enabled(void);
void enable_stdout_logging(int stdoutEnabled);

void set_log_thread(Thread *thread);
void set_log_thread_callback(NotifyThreadFunc func);
pthread_mutex_t * get_log_mutex(void);
void set_log_pending(int pending);
void notify_log_pending(void);

#ifdef TEST

FILE * open_log_file(char *dirPath, char *identifier);
void finalize_logging(void);

#endif

#endif