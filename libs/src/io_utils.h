#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "common.h"

#include <stdbool.h>
#include <sys/types.h>

#define READ_PIPE 0
#define WRITE_PIPE 1
#define PIPE_FD_COUNT 2

typedef struct StreamPipe StreamPipe;

/* create pipe and set its reading and writing
    fd's to non-blocking mode */
StreamPipe * create_pipe(void);
void delete_pipe(StreamPipe *streamPipe);

void reset_pipe_buffer(StreamPipe *streamPipe);

ssize_t read_string(int fd, char *buffer, int size);
ssize_t write_string(int fd, const char *string);

/* strip string of a CRLF sequence */
int read_message(int fd, char *buffer, int size);

/* add CRLF sequence to the string */
int write_message(int fd, const char *message);

int get_pipe_fd(StreamPipe *streamPipe, int pipeIdx);
char * get_pipe_buffer(StreamPipe *streamPipe);
int get_pipe_buffer_size(StreamPipe *streamPipe);

bool is_pipe_buffer_full(StreamPipe *streamPipe);

#endif