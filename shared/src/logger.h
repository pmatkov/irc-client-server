#ifndef LOGGER_H
#define LOGGER_H

#include "string_utils.h"
#include "error_control.h"

#include <stdio.h>

#ifdef TEST
    #define LOG_FILE(str) APPEND_STRING_PREFIX(test_, str)
#else
    #define LOG_FILE(str) TO_STRING(str)
#endif

#define LOG(level, msg, ...) \
    log_message(level, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    LOGLEVEL_COUNT
} LogLevel;

typedef struct Logger Logger;

const char * get_log_level_string(LogLevel logLevel);

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);
void log_error(const char *msg, ErrorCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

void set_stdout_allowed(int allowed);

#endif