#ifdef TEST
#include "priv_io_utils.h"
#include "mock.h"
#else
#include "io_utils.h"
#include "common.h"
#endif

#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct StreamPipe {
    int pipeFd[PIPE_FD_COUNT];
    char buffer[MAX_CHARS + 1];
};

#endif

StreamPipe * create_pipe(void) {

    StreamPipe *streamPipe = (StreamPipe*) malloc(sizeof(StreamPipe));
    if (streamPipe == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    if (pipe(streamPipe->pipeFd) == -1) {
        FAILED(NO_ERRCODE, "Error creating pipe");
    }

    for (int i = 0; i < PIPE_FD_COUNT; i++) {

        int flags = fcntl(streamPipe->pipeFd[i], F_GETFL);
        if (flags == -1) {
            FAILED(NO_ERRCODE, "Error getting fcntl");
        }

        if (fcntl(streamPipe->pipeFd[i], F_SETFL, flags | O_NONBLOCK) == -1) {
            FAILED(NO_ERRCODE, "Error setting fcntl");
        }
    }

    reset_pipe_buffer(streamPipe);

    return streamPipe;
}

void delete_pipe(StreamPipe *streamPipe) {

    if (streamPipe != NULL) {
        close(streamPipe->pipeFd[READ_PIPE]);
        close(streamPipe->pipeFd[WRITE_PIPE]);
    }

    free(streamPipe);
}

void reset_pipe_buffer(StreamPipe *streamPipe) {

    if (streamPipe == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    memset(streamPipe->buffer, '\0', ARRAY_SIZE(streamPipe->buffer));
}

ssize_t read_string(int fd, char *buffer, int size) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    /*  when reading from the socket, read() will return
        EOF when the peer socket is closed. if the peer 
        socket is closed, but the host tries to write
        more data to the socket, peer socket will respond 
        with RST and the read() will fail with ECONNRESET. 
        after RST is received, further write() will fail 
        with EPIPE. when reading from non-blocking pipe, 
        if the pipe is empty, read() will fail with 
        EAGAIN */
    ssize_t bytesRead = read(fd, buffer, size);

    return bytesRead;
}

ssize_t write_string(int fd, const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *strPtr = string;

    size_t bytesLeft = strlen(strPtr); 
    ssize_t bytesWritten = 0, totalBytesWritten = 0; 

    while (bytesLeft && totalBytesWritten != -1) { 

        bytesWritten = write(fd, strPtr, bytesLeft);

        if (bytesWritten > 0) {

            totalBytesWritten += bytesWritten;
            bytesLeft -= bytesWritten; 
            strPtr += bytesWritten; 
        }
        else {
            if (bytesWritten <= 0) {
                totalBytesWritten = -1;
            }
        }
    }
    return totalBytesWritten;
}

int read_message(int fd, char *buffer, int size) {

    int readStatus = read_string(fd, buffer, size);

    if (readStatus > 0) {

        clear_terminator(buffer, CRLF);
        readStatus = 1;
    }
    else if (readStatus == -1 && errno != EAGAIN) {
        FAILED(NO_ERRCODE, "Error reading message");
    }
    return readStatus;
}

int write_message(int fd, const char *message) {

    char fmtMessage[MAX_CHARS + 1] = {'\0'};
    
    terminate_string(fmtMessage, MAX_CHARS + 1, message, CRLF);

    int writeStatus = write_string(fd, fmtMessage);
    
    if (writeStatus == -1) {
        FAILED(NO_ERRCODE, "Error sending message");
    }
    else if (writeStatus) {
        writeStatus = 1;
    }
    return writeStatus;
}

int get_pipe_fd(StreamPipe *streamPipe, int pipeIdx) {

    if (streamPipe == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return streamPipe->pipeFd[pipeIdx];
}

char * get_pipe_buffer(StreamPipe *streamPipe) {

    if (streamPipe == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return streamPipe->buffer;
}

int get_pipe_buffer_size(StreamPipe *streamPipe) {
    
    if (streamPipe == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return ARRAY_SIZE(streamPipe->buffer); 
}

bool is_pipe_buffer_full(StreamPipe *streamPipe) {

    if (streamPipe == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return strlen(streamPipe->buffer) >= ARRAY_SIZE(streamPipe->buffer) - 1;   
}