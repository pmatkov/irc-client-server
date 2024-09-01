#ifndef LOGGER_H
#define LOGGER_H

#include "../src/error_control.h"

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

typedef struct {
    FILE *logFile;
    char **logBuffer;
    int stdoutAllowed;
    int allocatedLines;
    int usedLines;
} Logger;

Logger * create_logger(char *dirPath, char *identifier);
void delete_logger(Logger *logger);

void set_stdout_allowed(int allowed);
int is_stderr_allowed(void);
void set_stderr_allowed(int allowed);

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);
void log_error(const char *msg, ErrCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

const char * get_log_level_string(LogLevel logLevel);

#ifdef TEST

FILE * open_log_file(char *dirPath, char *identifier);
void write_log_to_file(void);

int is_dir(const char *dirName);
int create_dir(const char *dirName);

#endif

#endif