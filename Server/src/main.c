#include "main.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 50100
#define MAX_LINE 4096
#define LISTEN_QUEUE 1024
#define MAX_FD 50

void check_pfd_status(struct pollfd **pollFds, int pfdCount, int activePfds, int pfdEventCount);

void add_client (struct pollfd **, int, int *, int *);
void delete_client(struct pollfd **, int, int *);

void delete_pfds(struct pollfd *pollFds);

// free
#ifndef TEST

int main(void)
{
    // create servers' listening TCP socket
    int listenSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSockFd < 0) {
        failed("Error creating socket.");
    }

    struct sockaddr_in servaddr;

    memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    int enableSockOpt = 1;
    if (setsockopt(listenSockFd, SOL_SOCKET, SO_REUSEADDR, &enableSockOpt, sizeof(int)) < 0) {
        failed("Error setting socket option.");
    }

    if (bind(listenSockFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in)) < 0) {
        failed("Error binding address to socket.");
    }

    if (listen(listenSockFd, LISTEN_QUEUE) == -1) {
        failed("Error setting listening socket.");
    }

    int pfdEventCount;
    int pfdCount = MAX_FD;
    int activePfds = 0;

    struct pollfd *pollFds = (struct pollfd *) malloc(sizeof(struct pollfd) * pfdCount);

    pollFds[0].fd = listenSockFd;
    pollFds[0].events = POLLIN;
    activePfds++;

    // started address + port
    printf("Server started.\n");

    while (1) {

        // poll file descriptors for events
        if ((pfdEventCount = (poll(pollFds, pfdCount, -1))) < 0) {
            failed("Error polling sockets.");
        }

        check_pfd_status(&pollFds, pfdCount, activePfds, pfdEventCount);
    }

    delete_pfds(pollFds);

    return 0;
}

#endif

void check_pfd_status(struct pollfd **pollFds, int pfdCount, int activePfds, int pfdEventCount) {

    int listenEvent = 0;
    char clientIPv4[INET_ADDRSTRLEN];

    /* if activity was detected on server's listening socket,
    call accept and create new client connection socket */
    if (!listenEvent && pollFds[0]->revents & POLLIN) {

        struct sockaddr_in clientaddr;
        socklen_t clientLen;

        int connSockFd = accept(pollFds[0]->fd, (struct sockaddr*) &clientaddr, &clientLen);

        if (connSockFd < 0) {
            failed("Error accepting connection.");
        }

        // add new connection socket fd to list of poll fd's

        add_client(pollFds, connSockFd, &activePfds, &pfdCount);

        const char *translatedIPAddr = inet_ntop(AF_INET, &clientaddr, clientIPv4, INET_ADDRSTRLEN);

        if (!translatedIPAddr) {
            failed("Translation failed");
        }
        printf("New connection from %s on socket %d\n", clientIPv4, connSockFd);

        listenEvent = 1;
        pfdEventCount--;
    }

    for (int i = 1; i < activePfds && pfdEventCount; i++) {

        if (pollFds[i]->revents & (POLLIN | POLLERR)) {

            ssize_t bytesReceived;
            char buffer[MAX_LINE];

            if ((bytesReceived = read(pollFds[i]->fd, buffer, MAX_LINE)) <= 0) {

                if (!bytesReceived) {
                    printf("Client disconnected (fd: %d)\n", pollFds[i]->fd);
                }
                else {
                    failed("Error reading from socket.");
                }

                close(pollFds[i]->fd);
                delete_client(pollFds, i, &activePfds);
            }
            else {
                printf("Msg from client %s", buffer);

            }
            pfdEventCount--;
        }      
    }
}

void add_client(struct pollfd **pollFds, int connSockFd, int *activePfds, int *pfdCount) {

    if (pollFds == NULL || activePfds == NULL || pfdCount == NULL) {
        failed("Unable to add client.");
    }

    if (*activePfds == *pfdCount) {
        *pfdCount += MAX_FD;

        *pollFds = (struct pollfd*) realloc(*pollFds, sizeof(struct pollfd) * (*pfdCount));
    }

    for (int i = 1; i < *pfdCount; i++) {

        if (pollFds[i]->fd != -1) {

            (*pollFds)[*activePfds].fd = connSockFd;
            (*pollFds)[*activePfds].events = POLLIN;
            (*activePfds)++;
            break;
        }
    }
}

void delete_client(struct pollfd **pollFds, int index, int *activePfds) {

    pollFds[index] = pollFds[*activePfds-1];
    (*activePfds)--;
}

void delete_pfds(struct pollfd *pollFds) {
    free(pollFds);
}

void failed(const char *msg, ...) {

    // save error number before calling functions which could change it
    int errnosv = errno;

    // check for additional args
    if (strchr(msg, '%') == NULL) {
        fprintf(stderr, msg);
    } else { 
        va_list arglist;
        va_start(arglist, msg);

        vfprintf(stderr, msg, arglist);
        va_end(arglist);
    }

    if (errnosv) {
        fprintf(stderr, " (error code = %d: %s)\n", errnosv, strerror(errnosv));
    }
 
    exit(EXIT_FAILURE);
}
