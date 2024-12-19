#include "mock.h"

#include "string_utils.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

static int mockFd = -1;
static void *mockBuffer = NULL;
static size_t mockBufferSize = 0;
static int mockPort = 0;
static struct sockaddr_in *mockSockaddr = NULL;
static int mockWidth = 0;
static int mockHeight = 0;

static int fdCount = 0;

int get_mock_fd(void) {

    return mockFd;
}

void set_mock_fd(int fd) {

   mockFd = fd; 
}

size_t get_mock_buffer_size(void) {

    return mockBufferSize;
}

void set_mock_buffer_size(size_t len) {
    
    mockBufferSize = len;
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

ssize_t mock_read(int fd, void *buffer, size_t readBytes) {

    ssize_t bytesRead = -1;

    if (buffer != NULL && fd == mockFd) {

        if (readBytes > mockBufferSize) {
            readBytes = mockBufferSize;
        }
        memcpy(buffer, mockBuffer, readBytes); 

        bytesRead = (ssize_t) readBytes;
    }

    return bytesRead; 
}

ssize_t mock_write(int fd, const void *buffer, size_t writeBytes) {

    ssize_t bytesWritten = -1;

    if (buffer != NULL && fd == mockFd) {

        if (writeBytes > mockBufferSize) {
            writeBytes = mockBufferSize;
        }
        if (writeBytes > strlen(buffer)) {
            writeBytes = strlen(buffer);
        }
        memcpy(mockBuffer, buffer, writeBytes); 

        mockBuffer += writeBytes; 
        
        bytesWritten = (ssize_t) writeBytes;
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

    if (fd == mockFd && clientAddr != NULL && len != NULL) {

        struct sockaddr_in *mockAddr = (struct sockaddr_in *)clientAddr;
        mockAddr->sin_family = AF_INET;
        mockAddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        mockAddr->sin_port = htons(mockPort);
        *len = sizeof(struct sockaddr_in);

        connectFd = ++fdCount;
    }
    return connectFd;
}

bool mock_get_address(char *buffer, int size, int *port, int fd) {

    bool converted = 0; 

    if (mockSockaddr != NULL) {

        if (buffer != NULL) { 
            inet_ntop(AF_INET, &mockSockaddr->sin_addr, buffer, size);
            converted = 1;
        }

        if (port != NULL) {
            *port = ntohs(mockSockaddr->sin_port);
            converted = 1;
        }
    }

    return converted;
}

int mock_get_wheight(WINDOW *window) {

    return mockHeight;
}

int mock_get_wwidth(WINDOW *window) {
    
    return mockWidth;
}

WINDOW * mock_initscr(void) {

    return stdscr;
}

/* newterm sets the global values stdscr and 
    curscr, just like initscr(). when switching
    screens with set_term(), the pointers for 
    stdscr and curscr are updated */

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