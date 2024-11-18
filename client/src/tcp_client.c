#ifdef TEST
#include "priv_tcp_client.h"
#include "../../libs/src/mock.h"
#else
#include "tcp_client.h"
#endif

#include "../../libs/src/io_utils.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/string_utils.h"
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
#define STDIN_IDX 0
#define SOCKET_FD_IDX 1
#define UNASSIGNED_FD -1

#ifndef TEST

/*  struct pollfd's array contains stdin, socket and 
    pipe file descriptors. these are used by poll() to
    monitor input events on these I/O channels. inBuffer
    servers as temp buffer for reading incoming messages
    and msgQueue for queueing outgoing messages */
struct TCPClient {
    struct pollfd pfds[POLL_FD_COUNT];
    char servername[MAX_CHARS + 1];
    char inBuffer[MAX_CHARS + 1];
    Queue *msgQueue;
};

#endif

STATIC void register_connection(TCPClient *tcpClient, int clientFd, const char *address, int port);

TCPClient * create_client(void) {

    TCPClient *tcpClient = (TCPClient *) malloc(sizeof(TCPClient));
    if (tcpClient == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    /* input events on fd may be monitered by setting 
        the events field to POLLIN. unassigned fd's
        (value of -1) are ignored by poll() */
    tcpClient->pfds[STDIN_IDX].fd = STDIN_FILENO;
    tcpClient->pfds[STDIN_IDX].events = POLLIN;
    tcpClient->pfds[SOCKET_FD_IDX].fd = UNASSIGNED_FD;
    tcpClient->pfds[SOCKET_FD_IDX].events = POLLIN;
    tcpClient->pfds[PIPE_FD_IDX].fd = UNASSIGNED_FD;
    tcpClient->pfds[PIPE_FD_IDX].events = POLLIN;
    memset(tcpClient->servername, '\0', sizeof(tcpClient->servername));
    memset(tcpClient->inBuffer, '\0', sizeof(tcpClient->inBuffer));
    tcpClient->msgQueue = create_queue(MSG_QUEUE_LEN, sizeof(RegMessage));

    return tcpClient;
}

void delete_client(TCPClient *tcpClient) {

    if (tcpClient != NULL) {
        
        if (is_client_connected(tcpClient)) {
            close(tcpClient->pfds[SOCKET_FD_IDX].fd);
        }

        delete_queue(tcpClient->msgQueue);
    }

    free(tcpClient);
}

int client_connect(TCPClient *tcpClient, const char *address, int port)
{
    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
    int connStatus = 0;

    if (!is_valid_ip(address)) {

        /* convert hostname to ip address if address 
            is a hostname */
        if (hostname_to_ip(ipv4Address, sizeof(ipv4Address), address)) {
            address = ipv4Address;
        }
        else {
            LOG(ERROR, "Invalid hostname or address: %s", address);
            connStatus = -2;
        }
    }
    if (!is_valid_port(port)) {
        LOG(ERROR, "Invalid port: %d", port);
        connStatus = -3;
    }

    if (!connStatus) {
        /* create a tcp socket for client's connection
            to the server */
        int clientFd = socket(AF_INET, SOCK_STREAM, 0); 
        if (clientFd < 0) {
            FAILED(NO_ERRCODE, "Error creating socket");
        }

        /* initialize socket structure */
        struct sockaddr_in servaddr;

        memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        inet_pton(AF_INET, address, &servaddr.sin_addr);

        /* establish tcp connection to the server */
        int connStatus = connect(clientFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));

        if (!connStatus) {
            register_connection(tcpClient, clientFd, address, port);
        }
        else {
            LOG(ERROR, "Error connecting to server");
        }
    }

    return connStatus;
}

void client_disconnect(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    close(tcpClient->pfds[SOCKET_FD_IDX].fd);
    unset_fd(tcpClient, SOCKET_FD_IDX);

    LOG(INFO, "Disconnected from the server");
}

STATIC void register_connection(TCPClient *tcpClient, int clientFd, const char *address, int port) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    } 

    char servername[MAX_CHARS + 1] = {'\0'};

    ip_to_hostname(servername, sizeof(servername), address);
    set_servername(tcpClient, servername);

    /* add fd returned by socket() to the set 
        of poll fd's monitored by poll() */
    set_fd(tcpClient, SOCKET_FD_IDX, clientFd);
    
    LOG(INFO, "Connected to server at %s: %d", address, port);
}

int client_read(TCPClient *tcpClient) {
    
    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};
    int readStatus = 0;
    // int readStatus = 0;
    // char readBuffer[MAX_CHARS + 1] = {'\0'};

    // ssize_t bytesRead = read(tcpClient->pfds[SOCKET_FD_IDX].fd, readBuffer, MAX_CHARS);

    /* read data from the socket */
    ssize_t bytesRead = read_string(tcpClient->pfds[SOCKET_FD_IDX].fd, readBuffer, sizeof(readBuffer) - 1);

    if (bytesRead <= 0) {

        if (!bytesRead) {
            client_disconnect(tcpClient);
            LOG(INFO, "Server terminated");
            readStatus = -1;
        }
        else if (bytesRead < 0 && errno != EINTR) {
            client_disconnect(tcpClient);
            LOG(ERROR, "Error reading from socket (fd: %d)", tcpClient->pfds[SOCKET_FD_IDX].fd);

        }
    }
    else {

        /* the client may receive a partial message 
            from the server. in this case, the partial 
            message is saved in the buffer and only after
            the full message is received will it be 
            parsed */
        int msgLength = strlen(tcpClient->inBuffer);
        int spaceLeft = ARR_SIZE(tcpClient->inBuffer) - msgLength - 1;
        int copyBytes = spaceLeft >= strlen(readBuffer) ? strlen(readBuffer) : spaceLeft;
        strncpy(tcpClient->inBuffer + msgLength, readBuffer, copyBytes);
        msgLength += copyBytes;

        /* IRC messages are terminated with CRLF sequence 
            ("\r\n") */
        if (find_delimiter(tcpClient->inBuffer, CRLF) != NULL) {

            char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
            escape_crlf_sequence(escapedMsg, sizeof(escapedMsg), tcpClient->inBuffer);
            
            LOG(DEBUG, "Received message(s) \"%s\" from socket (fd: %d)", escapedMsg, tcpClient->pfds[SOCKET_FD_IDX].fd);
            readStatus = 1;
        }
        if (!readStatus && msgLength == MAX_CHARS) {
            memset(tcpClient->inBuffer, '\0', sizeof(tcpClient->inBuffer));
        }
    }
    return readStatus;
}

void client_write(TCPClient *tcpClient, int fd, const char *message) {

    if (tcpClient == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    // size_t bytesLeft; 
    // ssize_t bytesWritten; 

    char fmtMessage[MAX_CHARS + 1] = {'\0'};

    /* IRC messages are terminated with CRLF sequence 
        ("\r\n") */
    if (!is_terminated(message, CRLF)) {

        terminate_string(fmtMessage, MAX_CHARS + 1, message, CRLF);
        message = fmtMessage;
    }
    
    /* send data to the socket */
    ssize_t bytesWritten = write_string(fd, message);

    if (bytesWritten <= 0) {

        if (bytesWritten < 0 && errno == EPIPE) {

            client_disconnect(tcpClient);
            LOG(INFO, "Server terminated");
        }
        else if (!bytesWritten || (bytesWritten < 0 && errno != EINTR)) {

            client_disconnect(tcpClient);
            LOG(ERROR, "Error writing to socket: %d", fd);
        }
    }


    // if (!write_string(fd, message)) {
    //     FAILED(NO_ERRCODE, "Error writing to socket: %d", fd);
    // }

    // const char *msgPtr = message;
    // bytesLeft = strlen(msgPtr); 

    // while (bytesLeft) { 

    //     if ((bytesWritten = write(fd, msgPtr, bytesLeft)) <= 0) { 

    //         if (!bytesWritten) {
    //             LOG(ERROR, "Error writing to the socket (fd: %d)", fd);
    //         }
    //         else if (bytesWritten < 0 && errno == EINTR) {
    //             bytesWritten = 0;
    //         }
    //     } 
    //     bytesLeft -= bytesWritten; 
    //     msgPtr += bytesWritten; 
    // }

    /* escape CRLF sequence in order to display it 
        in log messages */
    char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
    escape_crlf_sequence(escapedMsg, sizeof(escapedMsg), message);
    
    LOG(DEBUG, "Sent message \"%s\" to the socket (fd: %d)", escapedMsg, fd);
}

void add_message_to_client_queue(TCPClient *tcpClient, void *message) {

    if (tcpClient == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    enqueue(tcpClient->msgQueue, message);
}

void * remove_message_from_client_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return dequeue(tcpClient->msgQueue);
}

struct pollfd * get_fds(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->pfds;
}

const char * get_servername(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->servername;
}

void set_servername(TCPClient *tcpClient, const char *servername) {

    if (tcpClient == NULL || servername == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    safe_copy(tcpClient->servername, MAX_CHARS + 1, servername);
}

Queue * get_client_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->msgQueue;
}

char * get_client_inbuffer(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->inBuffer;
}

int get_inbuffer_size(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return ARR_SIZE(tcpClient->inBuffer);
}

char get_char_from_inbuffer(TCPClient *tcpClient, int index) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->inBuffer[index];
}

void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    tcpClient->inBuffer[index] = ch;
}

void set_fd(TCPClient *tcpClient, int fdIndex, int fd) {
      
    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    tcpClient->pfds[fdIndex].fd = fd;
}

void unset_fd(TCPClient *tcpClient, int fdIndex) {
      
    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    tcpClient->pfds[fdIndex].fd = UNASSIGNED_FD;
}

int is_client_connected(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->pfds[SOCKET_FD_IDX].fd != UNASSIGNED_FD;
}

int is_stdin_event(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpClient->pfds[STDIN_IDX].revents & POLLIN;
}

int is_socket_event(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpClient->pfds[SOCKET_FD_IDX].revents & POLLIN;
}

int is_pipe_event(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpClient->pfds[PIPE_FD_IDX].revents & POLLIN;
}

int get_stdin_fd(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpClient->pfds[STDIN_IDX].fd;
}

int get_socket_fd(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpClient->pfds[SOCKET_FD_IDX].fd;
}

int get_pipe_fd(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpClient->pfds[PIPE_FD_IDX].fd;
}
