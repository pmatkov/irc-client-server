#ifndef ERRORCTRL_H
#define ERRORCTRL_H

#define FAILED(msg, errorCode, ...) \
    failed(msg, errorCode,__func__, __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
    NO_ERRCODE,
    ARG_ERROR,
    IO_ERROR,
    UNKNOWN_ERROR,
    ERRCODE_COUNT
} ErrorCode;

const char * get_error_code_string(ErrorCode errorCode);
void set_stderr_allowed(int allowed);

void failed(const char *msg, ErrorCode errorCode, const char *function, const char *file, int line, ...);

#endif