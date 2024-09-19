#include "tcpclient.h"
#include "display.h"

#include "../../shared/src/priv_message.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/network_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

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

#define MAX_SERVERNAME_LEN 50
#define MAX_CHANNELNAME_LEN 50
#define MAX_CHARS 512
#define MSG_QUEUE_LEN 20

struct TCPClient {
    char servername[MAX_SERVERNAME_LEN + 1];
    char channelname[MAX_CHANNELNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    Queue *outQueue;
    int fd;
    int connected;
    int inChannel;
};

TCPClient * create_client(void) {

    TCPClient *tcpClient = (TCPClient *) malloc(sizeof(TCPClient));
    if (tcpClient == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    memset(tcpClient->inBuffer, '\0', MAX_CHARS + 1);

    tcpClient->outQueue = create_queue(MSG_QUEUE_LEN, sizeof(RegMessage));
    tcpClient->fd = -1;
    tcpClient->connected = 0;
    tcpClient->inChannel = 0;

    return tcpClient;
}

void delete_client(TCPClient *tcpClient) {

    if (tcpClient != NULL) {

        delete_queue(tcpClient->outQueue);
    }
    free(tcpClient);
}

int connect_to_server(TCPClient *tcpClient, char *address, char *port)
{
    char *savedAddress = address;
    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    // convert hostname to ip address
    if (!is_ipv4address(address)) {

        if (!convert_hostname_to_ip(address, ipv4Address, sizeof(ipv4Address))) {
            return -2;
        }
        else {
            address = ipv4Address;
        }
    }
    if (!is_port(port)) {
        return -3;
    }

    // create client's TCP socket
    int clientFd = socket(AF_INET, SOCK_STREAM, 0); 
    if (clientFd < 0) {
        FAILED("Error creating socket", NO_ERRCODE);
    }

    // initialize socket address structure with server's IP address and port
    struct sockaddr_in servaddr;

    memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(str_to_uint(port));

    inet_pton(AF_INET, address, &servaddr.sin_addr);

    // connect to server
    int connStatus = connect(clientFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));

    if (connStatus == 0) {
        
        client_set_fd(tcpClient, clientFd);
        client_set_servername(tcpClient, savedAddress);
        client_set_connected(tcpClient, 1);
        
        LOG(INFO, "Connected to %s: %s", address, port);
    }
   
    return connStatus;
}

int client_read(TCPClient *tcpClient) {
    
    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};

    ssize_t bytesRead = read(client_get_fd(tcpClient), readBuffer, MAX_CHARS);

    if (bytesRead <= 0) {

        if (!bytesRead) {
            LOG(INFO, "Server terminated");
        }
        else if (errno != ECONNRESET) {
            FAILED("Error reading from socket: %d", NO_ERRCODE, client_get_fd(tcpClient));
        }
        close(client_get_fd(tcpClient));
    }
    else {

        int msgLength = strlen(client_get_buffer(tcpClient));

        if (msgLength + bytesRead <= MAX_CHARS) {

            strcpy(client_get_buffer(tcpClient) + msgLength, readBuffer);
            msgLength += bytesRead;
        }

        // full message received
        if (client_get_char_in_buffer(tcpClient, msgLength - 1) == '\n') {
            LOG(INFO, "Message received from fd: %d", client_get_fd(tcpClient));

            return 1;
        }
    }
    return 0;
}

void client_write(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    while (!is_queue_empty(client_get_queue(tcpClient))) {

        size_t nleft; 
        ssize_t nwritten; 

        RegMessage *message = remove_message_from_client_queue(client_get_queue(tcpClient));

        char *msgPtr = get_reg_message_content(message);

        int len = strlen(msgPtr);

        if (msgPtr[len-1] != '\n') {
            msgPtr[len] = '\n';
            msgPtr[len + 1] = '\0';
        }

        nleft = strlen(msgPtr); 

        while (nleft) { 

            if ((nwritten = write(client_get_fd(tcpClient), msgPtr, nleft)) <= 0) { 

                if (nwritten < 0 && errno == EINTR) {
                    nwritten = 0;
                }
                else {
                    LOG(ERROR, "Error writing to socket: %d", client_get_fd(tcpClient));
                }
            } 
            nleft -= nwritten; 
            msgPtr += nwritten; 
        }
    }
}

Queue * client_get_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->outQueue;
}

char * client_get_buffer(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->inBuffer;
}

char client_get_char_in_buffer(TCPClient *tcpClient, int index) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->inBuffer[index];
}

void client_set_char_in_buffer(TCPClient *tcpClient, char ch, int index) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->inBuffer[index] = ch;
}

int client_get_fd(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->fd;
}

void client_set_fd(TCPClient *tcpClient, int fd) {
      
    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->fd = fd;
}

int client_is_connected(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->connected;
}

void client_set_connected(TCPClient *tcpClient, int connected) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->connected = connected;
}

int client_is_inchannel(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->inChannel;
}

void client_set_inchannel(TCPClient *tcpClient, int inChannel) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    tcpClient->inChannel = inChannel;
}

const char * client_get_servername(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->servername;
}

void client_set_servername(TCPClient *tcpClient, const char *servername) {

    if (tcpClient == NULL || servername == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    safe_copy(tcpClient->servername, MAX_SERVERNAME_LEN + 1, servername);
}

const char * client_get_channelname(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpClient->channelname;
}

void client_set_channelname(TCPClient *tcpClient, const char *channelname) {

    if (tcpClient == NULL || channelname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    safe_copy(tcpClient->channelname, MAX_CHANNELNAME_LEN + 1, channelname);
}

void add_message_to_client_queue(TCPClient *tcpClient, void *message) {

    if (tcpClient == NULL || message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    enqueue(tcpClient->outQueue, message);
}

void * remove_message_from_client_queue(Queue *queue) {

    if (queue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return dequeue(queue);
}
