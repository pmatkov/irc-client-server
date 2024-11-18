#ifndef LOGGER_H
#define LOGGER_H

#include "threads.h"
#include "string_utils.h"
#include "error_control.h"

#include <stdio.h>
#include <pthread.h>

/* generate a log file name by adding a prefix to the
    base name */
#ifdef TEST
    #define LOG_FILE(str) APPEND_STRING_PREFIX(test_, str)
#else
    #define LOG_FILE(str) TO_STRING(str)
#endif

#define LOG(level, msg, ...) \
    log_message(level, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

/* represents log levels used by the logger */
typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    UNKNOWN_LOGLEVEL,
    LOGLEVEL_COUNT
} LogLevel;

typedef void (*LogFunc)(void *arg);

/* represents container for managing logging */
typedef struct Logger Logger;

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

void write_log_to_file(void);

/* log standard message */
void log_message(LogLevel level, const char *msg, const char *function, const char *file, int line, ...);

/* log error message */
void log_error(ErrorCode errorCode, const char *msg, const char *function, const char *file, int line, int errnosv, ...);

const char * log_level_to_string(LogLevel logLevel);
LogLevel string_to_log_level(const char *string);
int is_valid_log_level(LogLevel logLevel);

int is_stdout_enabled(void);

/* enable or disable the display of log
    messages in the terminal */
void enable_stdout_logging(int stdoutEnabled);

void set_log_thread(Thread *thread);
void set_log_thread_callback(NotifyThreadFunc func);
pthread_mutex_t * get_log_mutex(void);
void set_log_pending(int pending);
void notify_log_pending(void);

#endif