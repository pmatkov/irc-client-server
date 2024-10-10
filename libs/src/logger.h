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
    UNKNOWN_LOGLEVEL,
    LOGLEVEL_COUNT
} LogLevel;

typedef struct Logger Logger;

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);
void log_error(const char *msg, ErrorCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

void set_use_stdout(int useStdout);

const char * log_level_to_string(LogLevel logLevel);
LogLevel string_to_log_level(const char *string);

int is_valid_log_level(LogLevel logLevel);

#endif