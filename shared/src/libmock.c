#include "mock.h"

#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 512

int mockFd;
void * mockBuffP;
size_t mockLen;

char mockString[MAX_CHARS + 1] = {'\0'};

ssize_t mock_read(int fd, void *buffer, size_t len) {

    if (fd != mockFd) {
        return -1;
    }
    if (len > mockLen) {
        len = mockLen;
    }
    memcpy(buffer, mockBuffP, len); 

    return len; 
}

ssize_t mock_write(int fd, const void *buffer, size_t len) {

    if (fd != mockFd) {
        return -1;
    }
    if (len > mockLen) {
        len = mockLen;
    }
    if (len > strlen(buffer)) {
        len = strlen(buffer);
    }
    memcpy(mockBuffP, buffer, len); 

    mockBuffP += len; 

    return len; 
}

int mock_close(int fd) {

    if (fd != mockFd) {
        return -1;
    }
    return 0;
}
