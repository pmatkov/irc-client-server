#ifndef ERRORCTRL_H
#define ERRORCTRL_H

#define FAILED(msg, errorCode, ...) \
    failed(msg, errorCode,__func__, __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum {
    NO_ERRCODE,
    ARG_ERROR,
    IO_ERROR,
    CLIENT_ERROR,
    SERVER_ERROR,
    UNKNOWN_ERROR,
    ERRCODE_COUNT
} ErrCode;

void set_stderr_allowed(int allowed);
void failed(const char *msg, ErrCode errorCode, const char *function, const char *file, int line, ...);
const char * get_error_code_string(ErrCode errorCode);

#endif