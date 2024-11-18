#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <sys/types.h>

#define READ_PIPE 0
#define WRITE_PIPE 1
#define PIPE_FD_COUNT 2

/* create pipe and set its fd's to non-blocking mode */
void create_pipe(int *pipeFd);

ssize_t read_string(int fd, char *buffer, int size);
ssize_t write_string(int fd, const char *string);

/* wrappers for read_string and send_string functions. 
    each message is delimited with CRLF sequence */
int read_message(int fd, char *buffer, int size);
int write_message(int fd, const char *message);

#endif