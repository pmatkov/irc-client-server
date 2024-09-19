#ifndef LOGGER_H
#define LOGGER_H

#include "error_control.h"

#include <stdio.h>

#define LOG(level, msg, ...) \
    log_message(level, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    LOGLEVEL_COUNT
} LogLevel;

typedef struct {
    FILE *logFile;
    char **logBuffer;
    LogLevel logLevel;
    int stdoutAllowed;
    int allocatedLines;
    int usedLines;
} Logger;

const char * get_log_level_string(LogLevel logLevel);

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);
void log_error(const char *msg, ErrorCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

void set_stdout_allowed(int allowed);

#ifdef TEST

FILE * open_log_file(char *dirPath, char *identifier);
void write_log_to_file(void);
int is_valid_log_level(LogLevel logLevel);

#endif

#endif