#include "io_utils.h"
#ifdef TEST
#include "mock.h"
#endif

#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

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

void create_pipe(int *pipeFd) {

    if (pipe(pipeFd) == -1) {
        FAILED(NO_ERRCODE, "Error creating pipe");
    }

    for (int i = 0; i < PIPE_FD_COUNT; i++) {

        int flags = fcntl(pipeFd[i], F_GETFL);
        if (flags == -1) {
            FAILED(NO_ERRCODE, "Error getting fcntl");
        }

        if (fcntl(pipeFd[i], F_SETFL, flags | O_NONBLOCK) == -1) {
            FAILED(NO_ERRCODE, "Error setting fcntl");
        }
    }
}

ssize_t read_string(int fd, char *buffer, int size) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    ssize_t bytesRead = read(fd, buffer, size);

    /*  when reading from the socket, read() will return
        0 (EOF) when the peer TCP socket is closed (when closing 
        the socket, kernel sends a FIN packet to initiate
        the termination sequence). if the peer has already 
        closed the connection, but the host tries to write more
        data to the socket, the peer socket will respond with 
        RST packet and the read() will fail with errno set to
        ECONNRESET. after recieving RST packet, further write 
        will fail with the SIGPIPE signal. when reading from the 
        non-blocking pipe, read() will fail with errno set to
        EAGAIN if the pipe is empty */

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
