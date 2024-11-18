#include "error_control.h"
#include "string_utils.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>

/* translation of error codes to strings */
static const char *ERRCODE_STRINGS[] = {
    "No error",
    "Invalid argument(s)",
    "Invalid range",
    "Input/ output error",
    "Unknown error"
};

static int stderrEnabled = 1;

static_assert(ARR_SIZE(ERRCODE_STRINGS) == ERRCODE_COUNT, "Array size mismatch");

const char * get_error_code_string(ErrorCode errorCode) {

    const char *string = NULL;

    if (errorCode >= 0 && errorCode < ERRCODE_COUNT) {
        string = ERRCODE_STRINGS[errorCode];
    }
    return string;
}

void failed(ErrorCode errorCode, const char *msg, const char *function, const char *file, int line, ...) {

    /* save errno value */
    int errnosv = errno;

    va_list arglist;
    va_start(arglist, line);

    /* log error to the file */
    log_error(errorCode, msg, function, file, line, errnosv, arglist);

    /* display error in the terminal */
    if (stderrEnabled) {

        if (msg != NULL) {

            if (strchr(msg, '%') == NULL) {
                fprintf(stderr, "%s (%s)\n", msg, function);
            } else { 
                vfprintf(stderr, msg, arglist);
                fprintf(stderr, " (%s)\n", function);
            }
        }
        else if (errorCode != NO_ERRCODE) {
            fprintf(stderr, "%s (%s)\n", ERRCODE_STRINGS[errorCode], function);
        }

        if (errnosv) {
            fprintf(stderr, "Error code = %d: %s.\n", errnosv, strerror(errnosv));
        }
    }
    va_end(arglist);

    exit(EXIT_FAILURE);
}

int is_stderr_enabled(void) {

    return stderrEnabled;
}

void enable_stderr_logging(int allowed) {

    stderrEnabled = allowed;
}

