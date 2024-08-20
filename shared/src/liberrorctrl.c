#include "errorctrl.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

static int allowConsole = 1;

static const char *ERRCODE_STRING[] = {
    "No error",
    "Invalid argument(s)",
    "Input/ output error",
    "Client error",
    "Server errror",
    "Unknown error"
};

_Static_assert(sizeof(ERRCODE_STRING) / sizeof(ERRCODE_STRING[0]) == ERRCODE_COUNT, "Array size mismatch");

const char * get_error_code_string(ErrCode errorCode) {

    if (errorCode >= 0 && errorCode < ERRCODE_COUNT) {
        return ERRCODE_STRING[errorCode];
    }
    return NULL;
}

void disable_stderr_logging() {
    allowConsole = 0;
}

void failed(const char *msg, ErrCode errorCode, const char *function, const char *file, int line, ...) {

    // save error number
    int errnosv = errno;

    va_list arglist;
    va_start(arglist, line);

    log_error(msg, errorCode, function, file, line, errnosv, arglist);

    if (allowConsole) {

        if (msg != NULL) {

            if (strchr(msg, '%') == NULL) {
                fprintf(stderr, "%s (%s)\n", msg, function);
            } else { 
                vfprintf(stderr, msg, arglist);
                fprintf(stderr, " (%s)\n", function);
            }
        }
        else if (errorCode != NO_ERRCODE) {
            fprintf(stderr, "%s (%s)\n", ERRCODE_STRING[errorCode], function);
        }

        if (errnosv) {
            fprintf(stderr, "Error code = %d: %s.\n", errnosv, strerror(errnosv));
        }
    }
    va_end(arglist);

    exit(EXIT_FAILURE);
}