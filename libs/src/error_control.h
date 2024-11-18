#ifndef ERRORCTRL_H
#define ERRORCTRL_H

#define FAILED(errorCode, msg, ...) \
    failed(errorCode, msg, __func__, __FILE__, __LINE__, ##__VA_ARGS__)

/* represents error codes used for logging 
    error messages */
typedef enum {
    NO_ERRCODE,
    ARG_ERROR,
    RANGE_ERROR,
    IO_ERROR,
    UNKNOWN_ERROR,
    ERRCODE_COUNT
} ErrorCode;

const char * get_error_code_string(ErrorCode errorCode);

/* log error message */
void failed(ErrorCode errorCode, const char *msg, const char *function, const char *file, int line, ...);

int is_stderr_enabled(void);
/* enable or disable the display of error
    messages in the terminal */
void enable_stderr_logging(int allowed);

#endif