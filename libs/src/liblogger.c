#ifdef TEST
#include "priv_logger.h"
#else
#include "logger.h"
#endif

#include "path.h"
#include "string_utils.h"
#include "time_utils.h"

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_LINES 25
#define DEFAULT_LOG_DIR "/log/"
#define DEFAULT_LOG_ID "default"

#define MAX_FILE_NAME 100
#define MAX_PATH_LEN 64
#define MAX_IDENTIFIER 16

/* the logger contains a reference to the
    log file, the log level and a log
    buffer for MAX_LINES lines */

struct Logger {
    FILE *logFile;
    char **logBuffer;
    LogLevel logLevel;
    int stdoutAllowed;
    int capacity;
    int count;
};

STATIC FILE * open_log_file(char *dirPath, char *identifier);
STATIC void write_log_to_file(void);

static Logger *logger = NULL;

/* lookup table for translation of log
    levels to strings */
static const char *LOGLEVEL_STRINGS[] = {
    "debug",
    "info",
    "warning",
    "error",
    "unknown"
};

static_assert(ARR_SIZE(LOGLEVEL_STRINGS) == LOGLEVEL_COUNT, "Array size mismatch");

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel) {

    if (logger == NULL) {

        logger = (Logger *) malloc(sizeof(Logger));
        if (logger == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        } 

        logger->logBuffer = (char **) malloc(MAX_LINES * sizeof(char *));
        if (logger->logBuffer == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        }

        for (int i = 0; i < MAX_LINES; i++) {

            logger->logBuffer[i] = (char *) calloc(MAX_CHARS + 1, sizeof(char));
            if (logger->logBuffer[i] == NULL) {
                FAILED("Error allocating memory", NO_ERRCODE);
            }
        }

        logger->logLevel = logLevel;
        logger->stdoutAllowed = 1;
        logger->capacity = MAX_LINES;
        logger->count = 0;

        FILE *fp = open_log_file(dirPath, identifier);

        logger->logFile = fp;
    }
    return logger;
}

void delete_logger(Logger *logger) {

    if (logger != NULL) {
        
        if (logger->count) {
            write_log_to_file();
        }
        
        if (logger->logFile != NULL) {
            fclose(logger->logFile);
        }

        if (logger->logBuffer != NULL) {

            for (int i = 0; i < MAX_LINES; i++) {
                free(logger->logBuffer[i]);
            }
        }
        free(logger->logBuffer);
    }
    free(logger);
}

/* open a log file for logging. if dirPath is 
    NULL, a default log dir will be set 
    (or created if it doesn't exist). if 
    identifier is NULL, a default identifier
    will be set */

STATIC FILE * open_log_file(char *dirPath, char *identifier) {

    char basePath[MAX_PATH_LEN + 1] = {'\0'};

    if (dirPath == NULL) {

        if (create_path(basePath, MAX_PATH_LEN, DEFAULT_LOG_DIR)) {
            dirPath = basePath;
        }
        else {
            FAILED("Error creating path", NO_ERRCODE);
        }
    }

    if (identifier == NULL) {
        identifier = DEFAULT_LOG_ID;
    }

    if (!is_dir(dirPath)) {
        create_dir(dirPath);
    }

    /* log file name format: <dir path>/<date>_<identifier>.log */
    char logFileName[MAX_FILE_NAME + 1] = {'\0'};

    /* set dir path */
    strncat(logFileName, dirPath, MAX_PATH_LEN);

    /* set date prefix */
    get_datetime(get_format_function(DATE), logFileName + strlen(logFileName), DATE_LENGTH);

    /* set identifier */
    strcat(logFileName, "_");
    strncat(logFileName, identifier, MAX_IDENTIFIER);
    strcat(logFileName, ".log");

    FILE *fp = fopen(logFileName, "a");
    
    if (fp == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    return fp;
}

/* write logs from log buffer to
    file */
STATIC void write_log_to_file(void) {

    if (logger != NULL && logger->logFile != NULL) {

        for (int i = 0; i < logger->count; i++) {
            fprintf(logger->logFile, logger->logBuffer[i]);
        }

        fflush(logger->logFile);

        logger->count = 0;  
    }
}

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...) { 

    if (logger == NULL || !is_valid_log_level(logger->logLevel) || logger->logLevel > level) {
        return;
    }

    char timestamp[DATETIME_LENGTH] = {'\0'};
    get_datetime(get_format_function(DATETIME), timestamp, DATETIME_LENGTH);

    if (msg != NULL) {

        /* log message format: <timestamp> <log level> <function> <message> */
        char *logLine = logger->logBuffer[logger->count];

        snprintf(logLine, MAX_CHARS + 1, "%s [%s] (%s) ", timestamp, LOGLEVEL_STRINGS[level], func);

        int remainingLen = remainingLen;

        if (strchr(msg, '%') == NULL) {

            /* log message without additional arguments */
            if (logger->stdoutAllowed) {
                fprintf(stdout, "%s\n", msg);
            }
            snprintf(&logLine[strlen(logLine)], remainingLen, "%s", msg);
        }
        else {

            /* log message with additional arguments */
            va_list arglist, arglistcp;
            va_start(arglist, line);
            va_copy(arglistcp, arglist); 

            /* display log messages in the terminal */
            if (logger->stdoutAllowed) {
                vfprintf(stdout, msg, arglistcp);
                fprintf(stdout, "\n");
            }

            vsnprintf(&logLine[strlen(logLine)], remainingLen, msg, arglist);
            va_end(arglist);
        }
        snprintf(&logLine[strlen(logLine)], remainingLen, "\n");
        logger->count++;
    }

    if (logger->count >= MAX_LINES) {
        write_log_to_file();
    }
}

void log_error(const char *msg, ErrorCode errorCode, const char *func, const char *file, int line, int errnosv, ...) {

    if (logger == NULL) {
        return;
    }

    char timestamp[DATETIME_LENGTH] = {'\0'};
    get_datetime(get_format_function(DATETIME), timestamp, DATETIME_LENGTH);

    char *fileName = strrchr(file, '/');
    if (fileName == NULL) {
        fileName = "";
    }
    /* error message format: <timestamp> <log level> <function> <filename> <line> <message> */
    char *logLine = logger->logBuffer[logger->count];

    snprintf(logLine, MAX_CHARS + 1, "%s [%s] (%s, file: %s, ln: %d) ", timestamp, LOGLEVEL_STRINGS[ERROR], func, &fileName[1], line);

    int remainingLen = remainingLen;

    if (msg != NULL) {

        if (strchr(msg, '%') == NULL) {

            snprintf(&logLine[strlen(logLine)], remainingLen, msg);
        }

        else {
            va_list arglist;
            va_start(arglist, errnosv);

            vsnprintf(&logLine[strlen(logLine)], remainingLen, msg, arglist);
            va_end(arglist);
        }
    }
    else if (errorCode != NO_ERRCODE) {

        snprintf(&logLine[strlen(logLine)], remainingLen, "%s", get_error_code_string(errorCode));
    }
    else if (errnosv)  {
        snprintf(&logLine[strlen(logLine)], remainingLen, "%s (code = %d)", strerror(errnosv), errnosv);
    }

    snprintf(&logLine[strlen(logLine)], remainingLen, "\n");
    logger->count++;

    if (logger->count >= MAX_LINES) {
        write_log_to_file();
    }
}

void set_stdout_allowed(int stdoutAllowed) {

    if (logger == NULL ) {
        return;
    }
    logger->stdoutAllowed = stdoutAllowed;
}

const char * log_level_to_string(LogLevel logLevel) {

    if (is_valid_log_level(logLevel)) {
        return LOGLEVEL_STRINGS[logLevel];
    };

    return NULL;
}

LogLevel string_to_log_level(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    LogLevel logLevel = UNKNOWN_LOGLEVEL;

    for (int i = 0; i < LOGLEVEL_COUNT; i++) {

        if (strcmp(LOGLEVEL_STRINGS[i], string) == 0) {
            logLevel = (LogLevel) i;
        }
    }
    return logLevel;
}

int is_valid_log_level(LogLevel logLevel) {

    return logLevel >= 0 && logLevel < LOGLEVEL_COUNT - 1;

}