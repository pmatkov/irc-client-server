#ifdef TEST
#include "test_tcpserver.h"
#include "../../shared/src/mock.h"
#else
#include "tcpserver.h"
#endif

#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_NICKNAME_LEN 9
#define MAX_MSG_LEN 512

#define DEFAULT_FDS 1024
#define SERVER_PORT 50100
#define LISTEN_QUEUE 128

#ifndef TEST

struct Client {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char ipv4Address[INET_ADDRSTRLEN];
    int port;
    char msgBuffer[MAX_MSG_LEN + 1];
};

struct PollFds {
    struct pollfd *pfds;
    Client *clients;
    int allocatedPfds;
    int usedPfds;
};

#endif

STATIC Client * create_clients(int size);
STATIC void delete_clients(Client *clients);

STATIC int find_pfd_index(PollFds *pollFds, int fd);

STATIC int read_data(PollFds *pollFds, int i);
STATIC void write_data(PollFds *pollFds, int i);

int init_server(void) {

    // create servers' listening TCP socket
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        FAILED("Error creating socket", NO_ERRCODE);
    }

    /* allow socket to bind to the same address/ port combination
     (enables quick restart without waiting for 2x MSL) */
    int enableSockOpt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &enableSockOpt, sizeof(int)) < 0) {
        FAILED("Error setting socket option", NO_ERRCODE);
    }

    // set servers' socket address structure
    struct sockaddr_in servaddr;

    memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    // bind socket to address and port
    if (bind(listenFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in)) < 0) {
        FAILED("Error binding address to socket", NO_ERRCODE);
    }

    // designate socket as listening socket
    if (listen(listenFd, LISTEN_QUEUE) < 0) {
        FAILED("Error setting listening socket", NO_ERRCODE);  
    }

    char ipv4Address[INET_ADDRSTRLEN];

    LOG(INFO, "Server started at %s: %d", inet_ntop(AF_INET, &servaddr.sin_addr, ipv4Address, sizeof(ipv4Address)), ntohs(servaddr.sin_port));

    return listenFd;
}

PollFds * create_pfds(int size) {

    if (size <= 0) {
        size = DEFAULT_FDS;
    }

    PollFds *pollFds = (PollFds*) malloc(sizeof(PollFds));
    if (pollFds == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }
    pollFds->pfds = (struct pollfd *) malloc(size * sizeof(struct pollfd));
    if (pollFds->pfds == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    for (int i = 0; i < size; i++) {
        pollFds->pfds[i].fd = -1;
    }

    pollFds->clients = create_clients(size);

    pollFds->allocatedPfds = size;
    pollFds->usedPfds = 0;

    return pollFds;
}

void delete_pfds(PollFds *pollFds) {

    if (pollFds != NULL) {
        free(pollFds->pfds);
        delete_clients(pollFds->clients);
    }
    free(pollFds);
}

struct pollfd * get_pfds(PollFds *pollFds) {

    return pollFds->pfds;
}

int get_allocated_pfds(PollFds *pollFds) {
    
    return pollFds->allocatedPfds;
}

void check_listening_pfd(PollFds *pollFds, int *fdsReady) {

    if (pollFds == NULL || fdsReady == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (pollFds->usedPfds && pollFds->pfds[0].revents & POLLIN) {

        struct sockaddr_in clientaddr;
        socklen_t clientLen = sizeof(clientaddr);

        // accept connection request on listening socket
        int connectFd = accept(pollFds->pfds[0].fd, (struct sockaddr*) &clientaddr, &clientLen);

        if (connectFd < 0) {
            FAILED("Error accepting connection", NO_ERRCODE);
        }

        int index = find_pfd_index(pollFds, -1);

        if (index == -1) {
            FAILED("No available pfds", NO_ERRCODE);
        }

        set_pfd(pollFds, index, connectFd, POLLIN);

        // get client's IP address and port number
        inet_ntop(AF_INET, &clientaddr.sin_addr, pollFds->clients[index].ipv4Address, sizeof(pollFds->clients[index].ipv4Address));
        pollFds->clients[index].port = ntohs(clientaddr.sin_port); 

        LOG(INFO, "New connection (#%d) from client %s on port %d (fd: %d)", pollFds->usedPfds - 1, pollFds->clients[index].ipv4Address, pollFds->clients[index].port, pollFds->clients[index].fd);

        (*fdsReady)--;

    }
}

void check_connected_pfds(PollFds *pollFds, int *fdsReady, int echoServer) {

    if (pollFds == NULL || fdsReady == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    for (int i = 1; i < pollFds->allocatedPfds && *fdsReady; i++) {

        // check for events on connected sockets
        if (pollFds->pfds[i].revents & (POLLIN | POLLERR)) { 

            int fullMsg = read_data(pollFds, i);

            /* if echo server is active and full message 
            was received, echo message back to the client */
            if (fullMsg && echoServer) {
                write_data(pollFds, i);
            }

            (*fdsReady)--;
        }
    }
}

void set_pfd(PollFds *pollFds, int index, int fd, short events) {

    if (pollFds == NULL || (index < 0 || index >= pollFds->allocatedPfds)) {
        FAILED(NULL, ARG_ERROR);
    }

    pollFds->pfds[index].fd = fd;
    pollFds->pfds[index].events = events;
    pollFds->clients[index].fd = fd;

    pollFds->usedPfds++;
}

void unset_pfd(PollFds *pollFds, int index) {

    if (pollFds == NULL|| (index < 0 || index >= pollFds->allocatedPfds)) {
        FAILED(NULL, ARG_ERROR);
    }

    pollFds->pfds[index].fd = -1;
    pollFds->clients[index].fd = -1;

    pollFds->usedPfds--;
}

STATIC Client * create_clients(int size) {

    Client *clients = (Client *) malloc(size * sizeof(Client));
    if (clients == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    for (int i = 0; i < size; i++) {

        clients[i].fd = -1;
        memset(clients[i].nickname, '\0', MAX_NICKNAME_LEN + 1);
        memset(clients[i].msgBuffer, '\0', MAX_MSG_LEN + 1);
    }

    return clients;
}

STATIC void delete_clients(Client *clients) {

    free(clients);
}

STATIC int find_pfd_index(PollFds *pollFds, int fd) {

    if (pollFds == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    
    int index = 0, found = 0;
    while (index < pollFds->allocatedPfds) {

        if (pollFds->pfds[index].fd == fd) {
            found = 1;
            break;
        }
        index++;
    }

    return found ? index : -1;
}

STATIC int read_data(PollFds *pollFds, int i) {
    
    if (pollFds == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char readBuffer[MAX_MSG_LEN + 1] = {'\0'};

    ssize_t bytesRead = read(pollFds->pfds[i].fd, readBuffer, MAX_MSG_LEN);

    if (bytesRead <= 0) {

        if (!bytesRead) {
            LOG(INFO, "Client %s on port %d disconnected (fd: %d)", pollFds->clients[i].ipv4Address, pollFds->clients[i].port, pollFds->clients[i].fd);
        }
        else if (errno != ECONNRESET) {
            FAILED("Error reading from socket: %d", NO_ERRCODE, pollFds->pfds[i].fd);
        }
        close(pollFds->pfds[i].fd);
        unset_pfd(pollFds, i);
    }
    else {

        int msgLength = strlen(pollFds->clients[i].msgBuffer);

        if (msgLength + bytesRead <= MAX_MSG_LEN) {

            strcpy(pollFds->clients[i].msgBuffer + msgLength, readBuffer);
            msgLength += bytesRead;
        }

        // full message received
        if (pollFds->clients[i].msgBuffer[msgLength - 1] == '\n') {
            LOG(INFO, "Message received from fd: %d", pollFds->clients[i].fd);

            return 1;
        }
    }
    return 0;
}

STATIC void write_data(PollFds *pollFds, int i) {

    if (pollFds == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    size_t nleft; 
    ssize_t nwritten; 
    char *ptr = pollFds->clients[i].msgBuffer; 

    nleft = strlen(pollFds->clients[i].msgBuffer); 

    while (nleft) { 

        if ((nwritten = write(pollFds->clients[i].fd, ptr, nleft)) <= 0) { 

            if (nwritten < 0 && errno == EINTR) {
                nwritten = 0;
            }
            else {
                FAILED("Error writing to socket: %d", NO_ERRCODE, pollFds->pfds[i].fd);

            }
        } 
        nleft -= nwritten; 
        ptr += nwritten; 
    }

    // reset msg buffer
    pollFds->clients[i].msgBuffer[0] = '\0';

}

