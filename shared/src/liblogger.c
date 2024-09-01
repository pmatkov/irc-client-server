#ifdef TEST
#include "test_logger.h"
#else
#include "logger.h"
#endif

#include "time.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_LINES 25
#define MAX_CHARS 512

#define MAX_FILE_NAME 100
#define MAX_PATH 64
#define MAX_IDENTIFIER 16

struct Logger {
    FILE *logFile;
    char **logBuffer;
    int stdoutAllowed;
    int allocatedLines;
    int usedLines;
};

STATIC FILE * open_log_file(char *dirPath, char *identifier);
STATIC void write_log_to_file(void);

STATIC int is_dir(const char *dirName);
STATIC int create_dir(const char *dirName);

STATIC Logger *logger = NULL;

STATIC const char *LOGLEVEL_STRING[] = {
    "Info",
    "Debug",
    "Warning",
    "Error",
    "Unknown level"
};

_Static_assert(sizeof(LOGLEVEL_STRING) / sizeof(LOGLEVEL_STRING[0]) == LOGLEVEL_COUNT, "Array size mismatch");

Logger * create_logger(char *dirPath, char *identifier) {

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

    logger->stdoutAllowed = 1;
    logger->allocatedLines = MAX_LINES;
    logger->usedLines = 0;

    FILE *fp = open_log_file(dirPath, identifier);

    logger->logFile = fp;

    return logger;
}

void delete_logger(Logger *logger) {

    if (logger) {
        
        if (logger->usedLines) {
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

void set_stdout_allowed(int allowed) {

    if (logger == NULL ) {
        return;
    }
    logger->stdoutAllowed = allowed;
}

STATIC FILE * open_log_file(char *dirPath, char *identifier) {

    if (dirPath == NULL) {
        dirPath = "log/";
    }

    if (identifier == NULL) {
        identifier = "default";
    }

    if (!is_dir(dirPath)) {
        create_dir(dirPath);
    }

    // log file name format: <path>/<date_prefix>_<app_name>.log
    char logFileName[MAX_FILE_NAME + 1] = {'\0'};

    // set dir path
    strncat(logFileName, dirPath, MAX_PATH);

    // set date prefix
    get_datetime(get_format_function(DATE), logFileName + strlen(logFileName), DATE_LENGTH);

    // set identifier
    strcat(logFileName, "_");
    strncat(logFileName, identifier, MAX_IDENTIFIER);
    strcat(logFileName, ".log");

    FILE *fp = fopen(logFileName, "a");
    
    if (fp == NULL) {
        FAILED("Error opening file", NO_ERRCODE);
    }

    return fp;
}

STATIC void write_log_to_file(void) {

    if (logger != NULL) {

        for (int i = 0; i < logger->usedLines; i++) {
            fprintf(logger->logFile, logger->logBuffer[i]);
        }

        fflush(logger->logFile);

        logger->usedLines = 0;  
    }
}

STATIC int is_dir(const char *dirName) {

    int isdir = 0;

    DIR* dir = opendir(dirName);

    if (dir != NULL) {
        closedir(dir);
        isdir = 1;
    }
    return isdir;
}

STATIC int create_dir(const char *dirName) {

    return mkdir(dirName, 0700);   
}

void log_message(LogLevel level, const char *msg, const char *func, const char *file, int line, ...) { 

    if (logger == NULL || logger->logFile == NULL) {
        return;
    }

    char timestamp[DATETIME_LENGTH] = {'\0'};
    get_datetime(get_format_function(DATETIME), timestamp, DATETIME_LENGTH);

    if (msg != NULL) {

        snprintf(logger->logBuffer[logger->usedLines], MAX_CHARS + 1, "%s [%s] (%s) ", timestamp, LOGLEVEL_STRING[level], func);

        if (strchr(msg, '%') == NULL) {

            if (logger->stdoutAllowed) {
                fprintf(stdout, "%s\n", msg);
            }

            snprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, "%s", msg);
        }

        else {

            va_list arglist, arglistcp;
            va_start(arglist, line);
            va_copy(arglistcp, arglist); 

            if (logger->stdoutAllowed) {
                vfprintf(stdout, msg, arglistcp);
                fprintf(stdout, "\n");
            }

            vsnprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, msg, arglist);
            va_end(arglist);
        }
        snprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, "\n");
        logger->usedLines++;
    }

    if (logger->usedLines >= MAX_LINES) {
        write_log_to_file();
    }
}

void log_error(const char *msg, ErrCode errorCode, const char *func, const char *file, int line, int errnosv, ...) {

    if (logger == NULL || logger->logFile == NULL) {
        return;
    }

    char timestamp[DATETIME_LENGTH] = {'\0'};
    get_datetime(get_format_function(DATETIME), timestamp, DATETIME_LENGTH);

    char *fileName = strrchr(file, '/');
    if (fileName == NULL) {
        fileName = "";
    }

    snprintf(logger->logBuffer[logger->usedLines], MAX_CHARS + 1, "%s [%s] (%s, file: %s, ln: %d) ", timestamp, LOGLEVEL_STRING[ERROR], func, &fileName[1], line);

    if (msg != NULL) {

        if (strchr(msg, '%') == NULL) {

            snprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, msg);
        }

        else {
            va_list arglist;
            va_start(arglist, errnosv);

            vsnprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, msg, arglist);
            va_end(arglist);
        }
    }
    else if (errorCode != NO_ERRCODE) {

        snprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, "%s", get_error_code_string(errorCode));
    }
    else if (errnosv)  {
        snprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, "%s (code = %d)", strerror(errnosv), errnosv);
    }

    snprintf(&logger->logBuffer[logger->usedLines][strlen(logger->logBuffer[logger->usedLines])], MAX_CHARS + 1, "\n");
    logger->usedLines++;

    if (logger->usedLines >= MAX_LINES) {
        write_log_to_file();
    }
}

const char * get_log_level_string(LogLevel logLevel) {

    const char *string = NULL;

    if (logLevel >= 0 && logLevel < LOGLEVEL_COUNT) {
        string = LOGLEVEL_STRING[logLevel];
    }
    return string;
}