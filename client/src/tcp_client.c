#ifdef TEST
#include "priv_tcp_client.h"
#include "../../libs/src/mock.h"
#else
#include "tcp_client.h"
#include "../../libs/src/common.h"
#endif

#include "config.h"
#include "../../libs/src/common.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/message.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/enum_utils.h"
#include "../../libs/src/time_utils.h"
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

#define MAX_MESSAGES 20

#ifndef TEST

/*  pfds contains a set of file descriptors which 
    are monitored by poll() */
struct TCPClient {
    int fd;
    char serverIdentifier[MAX_CHARS + 1];
    HostIdentifierType identifierType;
    int port;
    char inBuffer[MAX_CHARS + 1];
    Queue *msgQueue;
    Timer *timer;
    SessionStateType clientState;
};

#endif

STATIC int validate_connection_params(const char *address, int port);
STATIC void initialize_session(TCPClient *tcpClient, int fd, const char *serverIdentifier, HostIdentifierType identifierType, int port);

TCPClient * create_client(void) {

    TCPClient *tcpClient = (TCPClient *) malloc(sizeof(TCPClient));
    if (tcpClient == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    tcpClient->fd = UNASSIGNED;
    memset(tcpClient->serverIdentifier, '\0', ARRAY_SIZE(tcpClient->serverIdentifier));
    tcpClient->identifierType = UNKNOWN_HOST_IDENTIFIER;
    tcpClient->port = UNASSIGNED;

    memset(tcpClient->inBuffer, '\0', sizeof(tcpClient->inBuffer));
    tcpClient->msgQueue = create_queue(MAX_MESSAGES, MAX_CHARS + 1);
    tcpClient->timer = create_timer();
    tcpClient->clientState = DISCONNECTED;

    return tcpClient;
}

void delete_client(TCPClient *tcpClient) {

    if (tcpClient != NULL) {
        
        delete_timer(tcpClient->timer);
        delete_queue(tcpClient->msgQueue);
    }

    free(tcpClient);
}

int client_connect(TCPClient *tcpClient, EventManager *eventManager, const char *address, int port)
{

    if (tcpClient == NULL || address == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int validationStatus = validate_connection_params(address, port);

    if (validationStatus == -2) {
        LOG(ERROR, "Invalid address: %s", address);
        return validationStatus;
    }
    else if (validationStatus == -3) {
        LOG(ERROR, "Invalid port: %d", port);
        return validationStatus;
    }

    /* create a tcp socket for client's connection
        to the server */
    int clientFd = socket(AF_INET, SOCK_STREAM, 0); 
    if (clientFd < 0) {
        FAILED(NO_ERRCODE, "Error creating socket");
    }

    /* initialize socket structure */
    struct sockaddr_in servaddr;

    set_sockaddr(&servaddr, address, port);

    /* establish tcp connection to the server */
    int connStatus = connect(clientFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));

    if (!connStatus) {

        char servername[MAX_CHARS + 1] = {'\0'};
        const char *serverIdentifier = address;
        HostIdentifierType identifierType = IP_ADDRESS;

        if (ip_to_hostname(servername, sizeof(servername), address)) {
            serverIdentifier = servername;
            identifierType = HOSTNAME;
        }

        initialize_session(tcpClient, clientFd, serverIdentifier, identifierType, port);

        if (eventManager != NULL) {
            Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_ADD_POLL_FD, .dataItem = (DataItem) {.itemInt = tcpClient->fd}, .dataType = INT_TYPE};
            push_event_to_queue(eventManager, &event);
        }
  
        if (is_allowed_state_transition(get_client_session_states(), get_client_state_type(tcpClient), CONNECTED)) {
            set_client_state_type(tcpClient, CONNECTED);
        }

        LOG(INFO, "Connecting to server at %s: %d", address, port);
    }
    else {
        LOG(ERROR, "Error connecting to server");
    }
    return connStatus;
}

void client_disconnect(TCPClient *tcpClient, EventManager *eventManager) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (eventManager != NULL) {

        push_event_to_queue(eventManager, &(Event){.eventType = NETWORK_EVENT, .subEventType = NE_CLIENT_DISCONNECT, .dataItem = (DataItem) {0}, .dataType = UNKNOWN_DATA_TYPE});
        push_event_to_queue(eventManager, &(Event){.eventType = NETWORK_EVENT, .subEventType = NE_REMOVE_POLL_FD, .dataItem = (DataItem) {.itemInt = tcpClient->fd}, .dataType = INT_TYPE});
    }

    if (is_allowed_state_transition(get_client_session_states(), get_client_state_type(tcpClient), DISCONNECTED)) {
        set_client_state_type(tcpClient, DISCONNECTED);
    }
}

STATIC int validate_connection_params(const char *address, int port) {
    
    int validationStatus = 0;

    if (!is_valid_ip(address)) {
        validationStatus = -2;
    }
    else if (!is_valid_port(port)) {
        validationStatus = -3;
    }

    return validationStatus;
}

STATIC void initialize_session(TCPClient *tcpClient, int fd, const char *serverIdentifier, HostIdentifierType identifierType, int port) {

    if (tcpClient == NULL || serverIdentifier == NULL || !is_valid_enum_type(identifierType, HOST_IDENTIFIER_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }

    tcpClient->fd = fd;
    safe_copy(tcpClient->serverIdentifier, ARRAY_SIZE(tcpClient->serverIdentifier), serverIdentifier);
    tcpClient->identifierType = identifierType;
    tcpClient->port = port;

}

void terminate_session(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    tcpClient->fd = UNASSIGNED;
    memset(tcpClient->serverIdentifier, '\0', ARRAY_SIZE(tcpClient->serverIdentifier));
    tcpClient->identifierType = UNKNOWN_HOST_IDENTIFIER;
    tcpClient->port = UNASSIGNED;
}


int client_read(TCPClient *tcpClient, EventManager *eventManager) {
    
    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};
    int readStatus = 0;

    /* read data from the socket */
    ssize_t bytesRead = read_string(tcpClient->fd, readBuffer, sizeof(readBuffer) - 1);

    if (bytesRead <= 0) {

        if (!bytesRead) {
            client_disconnect(tcpClient, eventManager);
            LOG(INFO, "Server terminated");
            readStatus = -1;

        }
        else if (bytesRead < 0 && errno != EINTR) {
            client_disconnect(tcpClient, eventManager);
            LOG(ERROR, "Error reading from socket (fd: %d)", tcpClient->fd);
        }
    }
    else {

        /* the client may receive a partial message 
            from the server. in this case, the partial 
            message is saved in the buffer and only after
            the full message is received will it be 
            parsed */
        int currentLen = strlen(tcpClient->inBuffer);
        int totalLen = currentLen + strlen(readBuffer);

        int copyBytes = totalLen >= MAX_CHARS + 1 ? MAX_CHARS - currentLen: strlen(readBuffer);
        safe_copy(tcpClient->inBuffer + currentLen, copyBytes + 1, readBuffer);
        currentLen += copyBytes;

        /* IRC messages are terminated with CRLF sequence 
            ("\r\n") */
        if (find_delimiter(tcpClient->inBuffer, CRLF) != NULL) {

            char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
            escape_crlf_sequence(escapedMsg, sizeof(escapedMsg), tcpClient->inBuffer);
            
            LOG(DEBUG, "Received message(s) \"%s\" from the server via socket (fd: %d)", escapedMsg, tcpClient->fd);
            readStatus = 1;
        }
        if (!readStatus && currentLen == MAX_CHARS) {
            memset(tcpClient->inBuffer, '\0', sizeof(tcpClient->inBuffer));
        }
    }
    return readStatus;
}

void client_write(TCPClient *tcpClient, EventManager *eventManager, const char *message) {

    if (tcpClient == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char fmtMessage[MAX_CHARS + 1] = {'\0'};

    /* IRC messages are terminated with CRLF sequence 
        ("\r\n") */
    if (!is_terminated(message, CRLF)) {
        terminate_string(fmtMessage, sizeof(fmtMessage), message, CRLF);
        message = fmtMessage;
    }
    
    /* send data to the socket */
    ssize_t bytesWritten = write_string(tcpClient->fd, message);

    if (bytesWritten <= 0) {

        if (bytesWritten < 0 && errno == EPIPE) {

            client_disconnect(tcpClient, eventManager);
            LOG(INFO, "Server terminated");
        }
        else if (!bytesWritten || (bytesWritten < 0 && errno != EINTR)) {

            client_disconnect(tcpClient, eventManager);
            LOG(ERROR, "Error writing to socket: %d", tcpClient->fd);
        }
    }
    else {
        /* escape CRLF sequence in order to display it 
            in log messages */
        char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
        escape_crlf_sequence(escapedMsg, sizeof(escapedMsg), message);
        
        LOG(DEBUG, "Sent message \"%s\" to the server via socket (fd: %d)", escapedMsg, tcpClient->fd);
    }
}

void enqueue_to_client_queue(TCPClient *tcpClient, void *message) {

    if (tcpClient == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    enqueue(tcpClient->msgQueue, message);
}

void * dequeue_from_client_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return dequeue(tcpClient->msgQueue);
}

int get_client_fd(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->fd;
}

void set_client_fd(TCPClient *tcpClient, int fd) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    tcpClient->fd = fd;
}

const char * get_server_identifier(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->serverIdentifier;
}

void set_server_identifier(TCPClient *tcpClient, const char *serverIdentifier, HostIdentifierType identifierType) {
    
    if (tcpClient == NULL || serverIdentifier == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    safe_copy(tcpClient->serverIdentifier, ARRAY_SIZE(tcpClient->serverIdentifier), serverIdentifier);
    tcpClient->identifierType = identifierType;
}

char * get_client_inbuffer(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->inBuffer;
}

void set_client_inbuffer(TCPClient *tcpClient, const char *string) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    safe_copy(tcpClient->inBuffer, ARRAY_SIZE(tcpClient->inBuffer), string);
}

Queue * get_client_queue(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->msgQueue;
}

SessionStateType get_client_state_type(TCPClient *tcpClient) {
    
    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->clientState;
}

void set_client_state_type(TCPClient *tcpClient, SessionStateType clientState) {
    
    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    tcpClient->clientState = clientState;
}

bool is_client_connected(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpClient->fd != UNASSIGNED;
}
