#ifdef TEST
#include "priv_tcp_client.h"
#include "../../libs/src/mock.h"
#else
#include "tcp_client.h"
#endif

#include "../../libs/src/settings.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MSG_QUEUE_LEN 20
#define FD_COUNT 2
#define STDIN_FD_INDEX 0
#define SOCKET_FD_INDEX 1

#ifndef TEST

/*  tcpClient contains an array of struct 
    pollfd's used by poll() to monitor the 
    state of file descriptors associated 
    with the specific I/O channel, such 
    as socket or stdin. the array contains
    stdin and tcp socket pollfd's. 
    additionally, tcpClient contains 
    inBuffer for storing incoming 
    messages and msgQueue for storing 
    outgoing messages. */
struct TCPClient {
    struct pollfd pfds[FD_COUNT];
    char serverName[MAX_CHARS + 1];
    char inBuffer[MAX_CHARS + 1];
    Queue *msgQueue;
};

#endif

TCPClient * create_client(void) {

    TCPClient *tcpClient = (TCPClient *) malloc(sizeof(TCPClient));
    if (tcpClient == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    /* input events on file descriptors are
        monitored by poll() by assigning 
        POLLIN to the events field. pollfd
        representing the stdin file 
        descriptor is stored at index
        [0] while the pollfd representing
        the tcp socket is stored at
        index [1] */

    tcpClient->pfds[0].fd = STDIN_FILENO;
    tcpClient->pfds[0].events = POLLIN;
    tcpClient->pfds[1].fd = -1;
    tcpClient->pfds[1].events = POLLIN;
    memset(tcpClient->serverName, '\0', sizeof(tcpClient->serverName));
    memset(tcpClient->inBuffer, '\0', sizeof(tcpClient->inBuffer));
    tcpClient->msgQueue = create_queue(MSG_QUEUE_LEN, sizeof(RegMessage));

    return tcpClient;
}

void delete_client(TCPClient *tcpClient) {

    if (tcpClient != NULL) {

        delete_queue(tcpClient->msgQueue);
    }
    free(tcpClient);
}

int client_connect(TCPClient *tcpClient, const char *hostOrAddr, const char *port)
{
    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    int connStatus = 0;

    if (!is_valid_ip_address(hostOrAddr)) {

        /* convert hostname to ipv4 address
            if hostOrAddr is a hostname */
        if (convert_hostname_to_ip_address(ipv4Address, sizeof(ipv4Address), hostOrAddr)) {
            hostOrAddr = ipv4Address;
        }
        else {
            connStatus = -2;
            LOG(ERROR, "Invalid hostname or address: %s", hostOrAddr);
        }
    }
    if (!is_valid_port(port)) {
        connStatus = -3;
        LOG(ERROR, "Invalid port: %s", port);
    }

    if (!connStatus) {

        /* creates a tcp socket and return
            file descriptor to that socket */
        int clientFd = socket(AF_INET, SOCK_STREAM, 0); 
        if (clientFd < 0) {
            FAILED("Error creating socket", NO_ERRCODE);
        }

        /* initialize socket address structure
            with server's address and port */
        struct sockaddr_in servaddr;

        memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(str_to_uint(port));

        inet_pton(AF_INET, hostOrAddr, &servaddr.sin_addr);

        /* create tcp connection to the server */
        connStatus = connect(clientFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));

        if (!connStatus) {

            char servername[MAX_CHARS + 1] = {'\0'};

            convert_ip_to_hostname(servername, MAX_CHARS + 1, hostOrAddr);
            set_server_name(tcpClient, servername);
    
            /* a file descriptor returned by socket()
                is added to the set of poll fd's 
                monitored by poll() */
            set_fd(tcpClient, SOCKET_FD_INDEX, clientFd);
            
            LOG(INFO, "Connected to the server at %s: %s", hostOrAddr, port);
        }
        else {
            FAILED("Error connecting to the server", NO_ERRCODE);
        }
    }
   
    return connStatus;
}


void client_disconnect(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    close(tcpClient->pfds[SOCKET_FD_INDEX].fd);
    unset_fd(tcpClient, SOCKET_FD_INDEX);

    LOG(INFO, "Disconnected from the server");
}

int client_read(TCPClient *tcpClient) {
    
    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};

    ssize_t bytesRead = read(tcpClient->pfds[SOCKET_FD_INDEX].fd, readBuffer, MAX_CHARS);

    /* bytesRead will be 0 if the server
        closes tcp connection */
    if (bytesRead <= 0) {

        if (!bytesRead) {
            LOG(INFO, "Server terminated");
        }
        else if (errno != ECONNRESET) {
            LOG(ERROR, "Error reading from socket (fd: %d)", tcpClient->pfds[SOCKET_FD_INDEX].fd);
        }
        client_disconnect(tcpClient);
    }
    else {

        /* the client may receive a partial message 
            from the server due to the nature of the
            tcp protocol. for this reason, all received
            data is stored in inBuffer, and only after
            the complete message is received (indicated
            by CRLF), will it be parsed */
        int msgLength = strlen(tcpClient->inBuffer);

        if (msgLength + bytesRead <= MAX_CHARS) {

            strcpy(tcpClient->inBuffer + msgLength, readBuffer);
            msgLength += bytesRead;
        }

        /* according to the IRC standard, all messages
           should be terminated with CRLF */
        if (is_crlf_terminated(tcpClient->inBuffer)) {
            LOG(DEBUG, "Received message \"%s\" from socket (fd: %d)", tcpClient->inBuffer, tcpClient->pfds[SOCKET_FD_INDEX].fd);

            return 1;
        }
    }
    return 0;
}

void client_write(const char *message, int fd) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    size_t bytesLeft; 
    ssize_t bytesWritten; 

    const char *msgPtr = message;
    char fmtMessage[MAX_CHARS + 1] = {'\0'};

    /* according to the IRC standard, all messages
        should be terminated with CRLF */
    if (!is_crlf_terminated(msgPtr)) {

        crlf_terminate(fmtMessage, MAX_CHARS + 1, msgPtr);
        msgPtr = fmtMessage;
    }

    bytesLeft = strlen(msgPtr); 

    while (bytesLeft) { 

        if ((bytesWritten = write(fd, msgPtr, bytesLeft)) <= 0) { 

            if (!bytesWritten) {
                LOG(ERROR, "Error writing to the socket (fd: %d)", fd);
            }
            else if (bytesWritten < 0 && errno == EINTR) {
                bytesWritten = 0;
            }
        } 
        bytesLeft -= bytesWritten; 
        msgPtr += bytesWritten; 
    }
    LOG(DEBUG, "Sent message \"%s\" to the socket (fd: %d)", message, fd);
}

void add_message_to_client_queue(TCPClient *tcpClient, void *message) {

    if (tcpClient == NULL || message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    enqueue(tcpClient->msgQueue, message);
}

void * remove_message_from_client_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return dequeue(tcpClient->msgQueue);
}

struct pollfd * get_fds(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->pfds;
}

const char * get_server_name(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->serverName;
}

void set_server_name(TCPClient *tcpClient, const char *serverName) {

    if (tcpClient == NULL || serverName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    safe_copy(tcpClient->serverName, MAX_CHARS + 1, serverName);
}

char * get_client_inbuffer(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->inBuffer;
}

char get_char_from_inbuffer(TCPClient *tcpClient, int index) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->inBuffer[index];
}

void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->inBuffer[index] = ch;
}

Queue * get_client_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->msgQueue;
}

void set_fd(TCPClient *tcpClient, int fdIndex, int fd) {
      
    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->pfds[fdIndex].fd = fd;
}

void unset_fd(TCPClient *tcpClient, int fdIndex) {
      
    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->pfds[fdIndex].fd = -1;
}

int is_client_connected(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->pfds[SOCKET_FD_INDEX].fd != -1;
}

int is_stdin_event(TCPClient *tcpClient) {

    return tcpClient->pfds[STDIN_FD_INDEX].revents & POLLIN;
}

int is_socket_event(TCPClient *tcpClient) {

    return tcpClient->pfds[SOCKET_FD_INDEX].revents & POLLIN;
}

int get_socket_fd(TCPClient *tcpClient) {

    return tcpClient->pfds[SOCKET_FD_INDEX].fd;
}