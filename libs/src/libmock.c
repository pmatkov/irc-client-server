#include "mock.h"

#include "string_utils.h"

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

static int mockFd;
static size_t mockLen;
static void *mockBuffer;
static int mockPort;
static struct sockaddr_in *mockSockaddr;
static WINDOW * mockStdscr;

static int fdCount = 0;

int get_mock_fd(void) {

    return mockFd;
}

void set_mock_fd(int fd) {

   mockFd = fd; 
}

size_t get_mock_len(void) {

    return mockLen;
}

void set_mock_len(size_t len) {
    
    mockLen = len;
}

void * get_mock_buffer(void) {

    return mockBuffer;
}

void set_mock_buffer(void *buffer) {

   mockBuffer = buffer; 
}

int get_mock_port(void) {

    return mockPort;
}

void set_mock_port(int port) {

   mockPort = port; 
}

struct sockaddr_in * get_mock_sockaddr(void) {

    return mockSockaddr;
}

void set_mock_sockaddr(struct sockaddr_in *sockaddr) {

   mockSockaddr = sockaddr; 
}

WINDOW * get_mock_stdscr(void) {

    return mockStdscr;
}

void set_mock_stdscr(WINDOW *stdscr) {

   mockStdscr = stdscr; 
}

ssize_t mock_read(int fd, void *buffer, size_t len) {

    ssize_t bytesRead = -1;

    if (buffer != NULL && fd == mockFd) {

        if (len > mockLen) {
            len = mockLen;
        }
        memcpy(buffer, mockBuffer, len); 

        bytesRead = (ssize_t) len;
    }

    return bytesRead; 
}

ssize_t mock_write(int fd, const void *buffer, size_t len) {

    ssize_t bytesWritten = -1;

    if (buffer != NULL && fd == mockFd) {

        if (len > mockLen) {
            len = mockLen;
        }
        if (len > strlen(buffer)) {
            len = strlen(buffer);
        }
        memcpy(mockBuffer, buffer, len); 

        mockBuffer += len; 
        
        bytesWritten = (ssize_t) len;
    }

    return bytesWritten; 
}

int mock_close(int fd) {

    int closeStatus = -1;

    if (fd == mockFd) {
        closeStatus = 0;
    }
    return closeStatus;
}

int mock_socket(int domain, int type, int protocol) {

    int fd = -1;

    if (domain == AF_INET && type == SOCK_STREAM && (protocol == AF_UNSPEC || protocol == AF_INET)) {
        fd = mockFd;
    } 

    return fd;
}

int mock_connect(int fd, struct sockaddr *servAddr, socklen_t len) {

    int status = -1;

    if (fd == mockFd && servAddr != NULL) {

        status = 0;
    }

    return status;
}

int mock_accept(int fd, struct sockaddr *clientAddr, socklen_t *len) {

    int connectFd = -1;

    if (fd == mockFd) {

        if (clientAddr != NULL && len != NULL) {

            struct sockaddr_in *mockAddr = (struct sockaddr_in *)clientAddr;
            mockAddr->sin_family = AF_INET;
            mockAddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            mockAddr->sin_port = htons(mockPort);
            *len = sizeof(struct sockaddr_in);

            connectFd = ++fdCount;
        }
    }

    return connectFd;
}

int mock_get_local_ip_address(char *buffer, int size, int fd) {

    int converted = 0; 

    if (buffer != NULL && mockSockaddr != NULL) {

        inet_ntop(AF_INET, &mockSockaddr->sin_addr, buffer, size);
        converted = 1; 
    }

    return converted;
}

WINDOW * mock_initscr(void) {

    return get_mock_stdscr();
}

SCREEN * create_terminal(void) {

    FILE *outfp = fopen("/dev/null", "w");
    FILE *infp = fopen("/dev/null", "r");

    SCREEN *screen = newterm(NULL, outfp, infp);
    set_term(screen); 

    return screen;
}

void delete_terminal(SCREEN *screen) {

    delscreen(screen);
}

void set_initial_fd_count(int count) {

    fdCount = count;
}