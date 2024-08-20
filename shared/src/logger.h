#ifndef LOGGER_H
#define LOGGER_H

#include "errorctrl.h"

#include <stdio.h>

#define MAX_LINES 25
#define MAX_CHARS 512

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

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
    int allocatedLines;
    int usedLines;
} Logger;

const char * get_log_level_string(LogLevel logLevel);
void disable_stdout_logging(void);

Logger * create_logger(char *dirPath, char *identifier);
void delete_logger(Logger *logger);

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);
void log_error(const char *msg, ErrCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

#ifdef TEST

STATIC FILE * open_log_file(char *dirPath, char *identifier);
STATIC void write_log_to_file(void);

STATIC int is_dir(const char *dirName);
STATIC int create_dir(const char *dirName);

#endif

#endif