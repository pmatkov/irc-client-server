#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_logger.h"
#else
#include "logger.h"
#endif

#include "common.h"
#include "enum_utils.h"
#include "settings.h"
#include "path.h"
#include "string_utils.h"
#include "time_utils.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

#define DEF_LOG_FREQUENCY 20
#define DEF_LOG_DIR "/log/"
#define DEF_LOG_ID "default"

#define MAX_FILE_NAME 100
#define MAX_PATH_LEN 64
#define MAX_IDENTIFIER 16

/* the logger contains a reference to the
    log file, the log level and a log
    buffer. the buffer size is limitied */
struct Logger {
    FILE *logFile;
    char **logBuffer;
    LogLevel logLevel;
    int logFrequency;
    bool stdoutEnabled;
    bool logPending;
    int currentCount;
    int totalCount;
    int capacity;
};

STATIC FILE * open_log_file(char *dirPath, char *identifier);
STATIC void finalize_logging(void);

static Logger *logger = NULL;

static NotifyThreadFunc notifyThreadFunc = NULL;
static Thread *logThread = NULL;
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

/* translation of log levels to strings */
static const char *LOGLEVEL_STRINGS[] = {
    "debug",
    "info",
    "warning",
    "error",
    "unknown"
};

ASSERT_ARRAY_SIZE(LOGLEVEL_STRINGS, LOGLEVEL_COUNT)

// static_assert(ARRAY_SIZE(LOGLEVEL_STRINGS) == LOGLEVEL_COUNT, "Array size mismatch");

Logger * create_logger(char *dirPath, char *identifier, LogLevel logLevel) {

    logger = (Logger *) malloc(sizeof(Logger));
    if (logger == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    } 

    logger->logBuffer = (char **) malloc(DEF_LOG_FREQUENCY * sizeof(char *));
    if (logger->logBuffer == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < DEF_LOG_FREQUENCY; i++) {

        logger->logBuffer[i] = (char *) calloc(MAX_CHARS + strlen(CRLF) + 1, sizeof(char));
        if (logger->logBuffer[i] == NULL) {
            FAILED(ALLOC_ERROR, NULL);
        }
    }

    logger->logLevel = logLevel;
    logger->logFrequency = logLevel == DEBUG ? 1 : DEF_LOG_FREQUENCY;
    logger->stdoutEnabled = 1;
    logger->logPending = 0;
    logger->currentCount = 0;
    logger->totalCount = 0;
    logger->capacity = DEF_LOG_FREQUENCY;

    FILE *fp = open_log_file(dirPath, identifier);

    logger->logFile = fp;
 
    return logger;
}

void delete_logger(Logger *logger) {

    if (logger != NULL) {
        
        finalize_logging();

        if (logger->logBuffer != NULL) {

            for (int i = 0; i < DEF_LOG_FREQUENCY; i++) {
                free(logger->logBuffer[i]);
            }
        }
        free(logger->logBuffer);
    }
    free(logger);
}

/* open a log file for logging. if dirPath is 
    NULL, a default log dir will be set (or
    created if it doesn't exist). if identifier
    is NULL, a default identifier will be set */
STATIC FILE * open_log_file(char *dirPath, char *identifier) {

    char basePath[MAX_PATH_LEN + 1] = {'\0'};

    if (dirPath == NULL) {

        if (create_path(basePath, MAX_PATH_LEN, DEF_LOG_DIR)) {
            dirPath = basePath;
        }
        else {
            FAILED(NO_ERRCODE, "Error creating path");
        }
    }

    if (identifier == NULL) {
        identifier = DEF_LOG_ID;
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
        FAILED(NO_ERRCODE, "Error opening file");
    }

    return fp;
}

STATIC void finalize_logging(void) {

    if (logger != NULL) {

        if (logger->currentCount) {
            write_log_to_file();
        }
        if (logger->logFile != NULL) {
            fclose(logger->logFile);
        }
    }
}

/* write log from the buffer to the file */
void write_log_to_file(void) {

    if (logger != NULL && logger->logFile != NULL) {

        if (logThread != NULL) {
            pthread_mutex_lock(&logMutex);
        }

        for (int i = 0; i < logger->currentCount; i++) {
            fprintf(logger->logFile, logger->logBuffer[i]);
        }

        fflush(logger->logFile);
        logger->currentCount = 0; 

        if (logThread != NULL) {
            pthread_mutex_unlock(&logMutex);
        }
    }
}

void log_message(LogLevel level, const char *msg, const char *function, const char *file, int line, ...) { 

    if (logger == NULL || !is_valid_enum_type(logger->logLevel, LOGLEVEL_COUNT) || logger->logLevel > level) {
        return;
    }

    if (msg != NULL) {

        char tsDateTime[DATETIME_LENGTH] = {'\0'};
        char tsHmsTime[HMS_TIME_LENGTH] = {'\0'};

        get_datetime(get_format_function(DATETIME), tsDateTime, DATETIME_LENGTH);
        get_datetime(get_format_function(HMS_TIME), tsHmsTime, HMS_TIME_LENGTH);

        /* log message format: <timestamp> <log level> <function> <message> */
        if (logThread != NULL) {
            pthread_mutex_lock(&logMutex);
        } 
        char *logLine = logger->logBuffer[logger->currentCount];

        if (logThread != NULL) {
            pthread_mutex_unlock(&logMutex);
        }

        snprintf(logLine, MAX_CHARS + 1, "%s [%s] (%s) ", tsDateTime, LOGLEVEL_STRINGS[level], function);

        int remainingLen = MAX_CHARS + 1 - strlen(logLine);

        if (strchr(msg, '%') == NULL) {

            /* print log message without arguments */
            if (logger->stdoutEnabled) {
                fprintf(stdout, "[%s] %s\n", tsHmsTime, msg);
            }
            snprintf(&logLine[strlen(logLine)], remainingLen, "%s", msg);
        }
        else {

            /* print log message with the arguments */
            va_list arglist, arglistcp;
            va_start(arglist, line);
            va_copy(arglistcp, arglist);

            /* display log messages in the terminal */
            if (logger->stdoutEnabled) {

                fprintf(stdout, "[%s] ", tsHmsTime);
                vfprintf(stdout, msg, arglistcp);
                fprintf(stdout, "\n");
            }

            vsnprintf(&logLine[strlen(logLine)], remainingLen, msg, arglist);
            va_end(arglist);

        }
        sprintf(&logLine[strlen(logLine)], "\n");

        if (logThread != NULL) {
            pthread_mutex_lock(&logMutex);
        } 

        logger->currentCount++;
        logger->totalCount++;

        if (logger->currentCount >= logger->logFrequency) {

            if (logThread == NULL) {
                write_log_to_file();
            }
            else {
                notify_log_pending();
                logger->logPending = 1;
            }
        }
        
        if (logThread != NULL) {
            pthread_mutex_unlock(&logMutex);
        }
    }
}

void log_error(ErrorCode errorCode, const char *msg, const char *function, const char *file, int line, int errnosv, ...) {

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
    if (logThread != NULL) {
        pthread_mutex_lock(&logMutex);
    } 
    char *logLine = logger->logBuffer[logger->currentCount];

    if (logThread != NULL) {
        pthread_mutex_unlock(&logMutex);
    }

    snprintf(logLine, MAX_CHARS + 1, "%s [%s] (%s, file: %s, ln: %d) ", timestamp, LOGLEVEL_STRINGS[ERROR], function, &fileName[1], line);

    int remainingLen = MAX_CHARS + 1 - strlen(logLine);

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

    sprintf(&logLine[strlen(logLine)], "\n");

    if (logThread != NULL) {
        pthread_mutex_lock(&logMutex);
    } 
    logger->currentCount++;
    logger->totalCount++;

    if (logger->currentCount >= logger->logFrequency) {
        if (!logThread) {
            write_log_to_file();
        }
        else {
            notify_log_pending();
            logger->logPending = 1;
        }
    }

    if (logThread != NULL) {
        pthread_mutex_unlock(&logMutex);
    }
}

const char ** get_log_level_strings(void) {

    return LOGLEVEL_STRINGS;
}

bool is_stdout_enabled(void) {

    bool enabled = 0;

    if (logger != NULL ) {
        enabled = logger->stdoutEnabled;
    }
    return enabled;
}

void enable_stdout_logging(int stdoutEnabled) {

    if (logger != NULL) {
        logger->stdoutEnabled = stdoutEnabled;
    }
}

void set_log_thread(Thread *thread) {

    logThread = thread;
}

void set_log_thread_callback(NotifyThreadFunc func) {

    notifyThreadFunc = func;
}

pthread_mutex_t * get_log_mutex(void) {

    return &logMutex;
}

void set_log_pending(int pending) {

    if (logger != NULL ) {

        if (logThread != NULL) {
            pthread_mutex_lock(&logMutex);
        }
        logger->logPending = pending;

        if (logThread != NULL) {
            pthread_mutex_unlock(&logMutex);
        }
    }
}

void notify_log_pending(void) {

    if (logger != NULL && !logger->logPending && notifyThreadFunc != NULL && logThread != NULL) {

        notifyThreadFunc(logThread, "log\r\n");
    }
}
