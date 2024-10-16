#ifndef ERRORCTRL_H
#define ERRORCTRL_H

/* macro which expands to failed function 
    call */
#define FAILED(msg, errorCode, ...) \
    failed(msg, errorCode,__func__, __FILE__, __LINE__, ##__VA_ARGS__)

/* errorCode represents error codes used by 
    logger */
typedef enum {
    NO_ERRCODE,
    ARG_ERROR,
    IO_ERROR,
    UNKNOWN_ERROR,
    ERRCODE_COUNT
} ErrorCode;

const char * get_error_code_string(ErrorCode errorCode);

/* log error message to the file or display it in 
    the terminal, if enabled. a message may 
    contain additional arguments specified 
    with format specifiers */
void failed(const char *msg, ErrorCode errorCode, const char *function, const char *file, int line, ...);

/* enable or disable output of the error
     messages in the terminal */
void set_stderr_allowed(int allowed);

#endif