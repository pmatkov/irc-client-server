#ifndef LOGGER_H
#define LOGGER_H

#include "string_utils.h"
#include "error_control.h"

#include <stdio.h>

/* macros used to create log file name */

#ifdef TEST
    #define LOG_FILE(str) APPEND_STRING_PREFIX(test_, str)
#else
    #define LOG_FILE(str) TO_STRING(str)
#endif

/* macro which expands to log_message function 
    call */
#define LOG(level, msg, ...) \
    log_message(level, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

/* LogLevel represents log levels used by
    logger */
typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    UNKNOWN_LOGLEVEL,
    LOGLEVEL_COUNT
} LogLevel;


/* logger contains all information relevant
    for logging */
typedef struct Logger Logger;

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel);
void delete_logger(Logger *logger);

/* log message to the file or display it in 
    the terminal if enabled. a message may 
    contain additional arguments specified 
    with format specifiers */
void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...);

/* log error message to a file or display it in 
    the terminal, if enabled. errors that occured 
    during system calls or standard library function
    calls are indicated by the variable errno and 
    are examined in this function */
void log_error(const char *msg, ErrorCode errorCode, const char *func, const char *file, int line, int errnosv, ...);

/* enable or disable output of the log
    messages in the terminal */
void set_stdout_allowed(int stdoutAllowed);

const char * log_level_to_string(LogLevel logLevel);
LogLevel string_to_log_level(const char *string);

int is_valid_log_level(LogLevel logLevel);

#endif