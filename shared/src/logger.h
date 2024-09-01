#ifndef LOGGER_H
#define LOGGER_H

#include "error_control.h"

#include <stdio.h>

#define LOG(level, msg, ...) \
    log_message(level, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
    INFO,
    DEBUG,
    WARNING,
    ERROR,
    UNKNOWN_LEVEL,
    LOGLEVEL_COUNT
} LogLevel;

typedef struct Logger Logger;

Logger * create_logger(char *dirPath, char *identifier);
void delete_logger(Logger *logger);

void set_stdout_allowed(int allowed);
int is_stderr_allowed(void);
void set_stderr_allowed(int allowed);

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);
void log_error(const char *msg, ErrCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

const char * get_log_level_string(LogLevel logLevel);

#endif