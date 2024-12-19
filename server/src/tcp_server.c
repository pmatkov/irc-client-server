#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_tcp_server.h"
#include "../../libs/src/mock.h"
#else
#include "tcp_server.h"
#include "../../libs/src/hash_table.h"
#endif

#include "config.h"

#include "../../libs/src/common.h"
#include "../../libs/src/session_state.h"
#include "../../libs/src/poll_manager.h"

#include "../../libs/src/io_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define LISTEN_QUEUE_LEN 50
#define MSG_QUEUE_LEN 50

#ifndef TEST

struct TCPServer {
    int listenFd;
    Client **clients;
    Session *session;
    Queue *outQueue;
    HashTable *fdsIdxMap;
    int count;
    int capacity;
    pthread_rwlock_t fdLock;
    pthread_rwlock_t queueLock;
    pthread_rwlock_t countLock;
};

#endif

STATIC int find_client_fd_idx(TCPServer *tcpServer, int fd);
STATIC void set_client_data(TCPServer *tcpServer, int fdIdx, int fd, const char *clientIdentifier, HostIdentifierType identifierType, int port);
STATIC void unset_client_data(TCPServer *tcpServer, int fdIdx);

TCPServer * create_server(int capacity) {

    TCPServer *tcpServer = (TCPServer*) malloc(sizeof(TCPServer));
    if (tcpServer == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    if (capacity <= 0) {
        capacity = DEF_FDS;
    }
    else if (capacity > MAX_FDS) {
        capacity = MAX_FDS;
    }

    tcpServer->listenFd = UNASSIGNED;

    tcpServer->clients = (Client**) malloc(capacity * sizeof(Client*));
    if (tcpServer->clients == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    for (int i = 0; i < capacity; i++) {
        tcpServer->clients[i] = create_client();
    }

    tcpServer->session = create_session();
    tcpServer->outQueue = create_queue(MSG_QUEUE_LEN, MAX_CHARS + 1);
    tcpServer->fdsIdxMap = create_hash_table(capacity, 0, fnv1a_hash, are_ints_equal, NULL, delete_pfd_idx_pair);
    tcpServer->count = 0;
    tcpServer->capacity = capacity;

    pthread_rwlock_init(&tcpServer->fdLock, NULL);
    pthread_rwlock_init(&tcpServer->queueLock, NULL);
    pthread_rwlock_init(&tcpServer->countLock, NULL);

    return tcpServer;
}

void delete_server(TCPServer *tcpServer) {

    if (tcpServer != NULL) {

        for (int i = 0; i < tcpServer->capacity; i++) {
            delete_client(tcpServer->clients[i]);
        }

        free(tcpServer->clients);
        delete_session(tcpServer->session);
        delete_queue(tcpServer->outQueue);
        delete_hash_table(tcpServer->fdsIdxMap);

        pthread_rwlock_destroy(&tcpServer->countLock);
        pthread_rwlock_destroy(&tcpServer->queueLock);
        pthread_rwlock_destroy(&tcpServer->fdLock);
    }
    free(tcpServer);
}

int init_server(TCPServer *tcpServer, const char *address, int port) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* create servers' listening TCP socket */
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        FAILED(NO_ERRCODE, "Error creating socket");
    }

    tcpServer->listenFd = listenFd;

    /* allow socket to bind to the same address/ port 
        (enables quick restart without waiting for 2x
        MSL) */
    int enableSockOpt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &enableSockOpt, sizeof(int)) < 0) {
        FAILED(NO_ERRCODE, "Error setting socket option");
    }

    /* initialize socket address structure */
    struct sockaddr_in servaddr;
    
    set_sockaddr(&servaddr, address, port);

    /* bind socket to an address and a port */
    if (bind(listenFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in)) < 0) {
        FAILED(NO_ERRCODE, "Error binding address to socket");
    }

    /* designate socket as a listening socket */
    if (listen(listenFd, LISTEN_QUEUE_LEN) < 0) {
        FAILED(NO_ERRCODE, "Error setting listening socket");  
    }

    char ipv4Address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &servaddr.sin_addr, ipv4Address, sizeof(ipv4Address));

    LOG(INFO, "Waiting for connection at %s: %d", ipv4Address, ntohs(servaddr.sin_port));

    return tcpServer->listenFd;
}

bool is_server_empty(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->countLock);
    }
    count = tcpServer->count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
    }

    return count == 0;
}

bool is_server_full(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->countLock);
    }
    count = tcpServer->count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
    }
    return count == tcpServer->capacity;
}

STATIC int find_client_fd_idx(TCPServer *tcpServer, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int index = UNASSIGNED;

    for (int i = 0; i < tcpServer->capacity; i++) {

        if (get_client_fd(tcpServer->clients[i]) == fd) {
            index = i;
            break;
        }
    }
    return index;
}

void add_client(TCPServer *tcpServer, EventManager *eventManager) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (!is_server_full(tcpServer) && tcpServer->listenFd != UNASSIGNED) {

        int clientFd = accept_connection(tcpServer);
        register_connection(tcpServer, eventManager, clientFd);
    }
}

int accept_connection(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct sockaddr_in clientaddr;
    socklen_t clientLen = sizeof(clientaddr);

    int clientFd = accept(tcpServer->listenFd, (struct sockaddr*) &clientaddr, &clientLen);

    if (clientFd < 0) {
        FAILED(NO_ERRCODE, "Error accepting connection");
    }
    return clientFd;
}   

void register_connection(TCPServer *tcpServer, EventManager *eventManager, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int fdIdx = find_client_fd_idx(tcpServer, UNASSIGNED);
    add_fd_idx_to_hash_table(tcpServer->fdsIdxMap, fd, fdIdx);

    char ipv4Address[INET_ADDRSTRLEN + 1] = {'\0'};
    int port;

    get_peer_address(ipv4Address, INET_ADDRSTRLEN, &port, fd);

    char servername[MAX_CHARS + 1] = {'\0'};
    const char *clientIdentifier = ipv4Address;
    HostIdentifierType identifierType = IP_ADDRESS;

    if (ip_to_hostname(servername, ARRAY_SIZE(servername), ipv4Address)) {
        clientIdentifier = servername;
        identifierType = HOSTNAME;
    }

    set_client_data(tcpServer, fdIdx, fd, clientIdentifier, identifierType, port);

    if (eventManager != NULL) {

        Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_ADD_POLL_FD, .dataItem = (DataItem) {.itemInt = fd}, .dataType = INT_TYPE};

        push_event_to_queue(eventManager, &event);   
    }

    if (is_allowed_state_transition(get_server_session_states(), get_client_state_type(tcpServer->clients[fdIdx]), CONNECTED)) {
        set_client_state_type(tcpServer->clients[fdIdx], CONNECTED);
    }

    tcpServer->count++;

    LOG(INFO, "New client connected (#%d) from %s: %d (fd: %d)", tcpServer->count, get_client_identifier(tcpServer->clients[fdIdx]), get_client_port(tcpServer->clients[fdIdx]), fd);
}

STATIC void set_client_data(TCPServer *tcpServer, int fdIdx, int fd, const char *clientIdentifier, HostIdentifierType identifierType, int port) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    set_client_fd(tcpServer->clients[fdIdx], fd);
    set_client_identifier(tcpServer->clients[fdIdx], clientIdentifier);
    set_client_identifier_type(tcpServer->clients[fdIdx], identifierType);
    set_client_port(tcpServer->clients[fdIdx], port);

}

STATIC void unset_client_data(TCPServer *tcpServer, int fdIdx) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    set_client_fd(tcpServer->clients[fdIdx], UNASSIGNED);
    set_client_identifier(tcpServer->clients[fdIdx], "");
    set_client_identifier_type(tcpServer->clients[fdIdx], UNKNOWN_HOST_IDENTIFIER);
    set_client_port(tcpServer->clients[fdIdx], UNASSIGNED);

}

void remove_client(TCPServer *tcpServer, EventManager *eventManager, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (!is_server_empty(tcpServer)) {

        int fdIdx = find_fd_idx_in_hash_table(tcpServer->fdsIdxMap, fd);
        remove_fd_idx_from_hash_table(tcpServer->fdsIdxMap, fd);

        unset_client_data(tcpServer, fdIdx);

        if (eventManager != NULL) {
            Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_REMOVE_POLL_FD, .dataItem = (DataItem) {.itemInt = fd}, .dataType = INT_TYPE};

            push_event_to_queue(eventManager, &event);
        }        
        
        if (is_allowed_state_transition(get_server_session_states(), get_client_state_type(tcpServer->clients[fdIdx]), DISCONNECTED)) {
            set_client_state_type(tcpServer->clients[fdIdx], DISCONNECTED);
        }

        tcpServer->count--;
    }

}

Client * find_client(TCPServer *tcpServer, const char *nickname) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Client *client = NULL;

    if (nickname != NULL) {

        if (get_int_option_value(OT_THREADS)) {
            pthread_rwlock_rdlock(&tcpServer->fdLock);
        }
        for (int i = 0; i < tcpServer->capacity; i++) {

            if (get_client_fd(tcpServer->clients[i]) != UNASSIGNED) {

                if (strcmp(get_client_nickname(tcpServer->clients[i]), nickname) == 0) {
                    client = tcpServer->clients[i];
                    break;
                }
            }
        }
        if (get_int_option_value(OT_THREADS)) {
            pthread_rwlock_unlock(&tcpServer->fdLock);
        }
    }
    return client;
}

int get_server_capacity(TCPServer *tcpServer) {
    
    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->capacity;
}

Client * get_client(TCPServer *tcpServer, int fdIdx) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->clients[fdIdx];
}

Session * get_session(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->session;
}

Queue * get_server_out_queue(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->outQueue;
}

HashTable * get_server_fds_idx_map(TCPServer *tcpServer) {
    
    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->fdsIdxMap;
}

void create_server_info(char *buffer, int size, void *arg) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *servername = get_char_option_value(OT_SERVER_NAME);
    
    if (servername != NULL) {
        safe_copy(buffer, size, servername);
    }
}

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content) {

    if (tcpServer == NULL || client == NULL || content == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_client_state_type(client) < REGISTERED) {

        char message[MAX_CHARS + 1] = {'\0'};
        char fdStr[MAX_DIGITS] = {'\0'};

        const char *tokens[] = {fdStr, "|", content};
        uint_to_str(fdStr, ARRAY_SIZE(fdStr), get_client_fd(client));
        concat_tokens(message, ARRAY_SIZE(message), tokens, ARRAY_SIZE(tokens), "");

        enqueue_to_server_queue(tcpServer, message);
    }
    else {

        const char *nickname = get_client_nickname(client);
        User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

        if (user != NULL) {

            enqueue_to_user_queue(user, (char*) content);
            add_user_to_ready_list(user, get_ready_list(get_session(tcpServer)));
        }
    }
}

void enqueue_to_server_queue(TCPServer *tcpServer, void *message) {

    if (tcpServer == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&tcpServer->queueLock);
    }
    enqueue(tcpServer->outQueue, message);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->queueLock);
    }
}

void * dequeue_from_server_queue(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    void *message = NULL;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&tcpServer->queueLock);
    }

    message = dequeue(tcpServer->outQueue);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->queueLock);
    }

    return message;
}

void add_irc_message_to_queue(TCPServer *tcpServer, Client *client, IRCMessage *tokens) {

    char message[MAX_CHARS - CRLF_LEN + 1] = {'\0'};

    create_irc_message(message, MAX_CHARS - CRLF_LEN, tokens);
    add_message_to_queue(tcpServer, client, message);
}

void send_message_to_user(void *user, void *arg) {

    if (user == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        TCPServer *tcpServer;
        EventManager *eventManager;
        const char *message;
    } *data = arg;

    server_write(data->tcpServer, data->eventManager, get_user_fd((User*)user), data->message);
}

void send_user_queue_messages(void *user, void *arg) {

    if (user == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        TCPServer *tcpServer;
        EventManager *eventManager;
        const char *message;
    } *data = arg;

    while ((data->message = dequeue_from_user_queue((User*)user)) != NULL) {

        server_write(data->tcpServer, data->eventManager, get_user_fd((User*)user), data->message);
    }
}

void send_channel_queue_messages(void *channel, void *arg) {

    if (channel == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        TCPServer *tcpServer;
        EventManager *eventManager;
        const char *message;
    } *data = arg;

    while ((data->message = dequeue_from_channel_queue((Channel*)channel)) != NULL) {

        Session *session = get_session(data->tcpServer);

        iterate_list(get_users_from_channel_users(find_channel_users(session, channel)), send_message_to_user, data);

    }
}

int server_read(TCPServer *tcpServer, EventManager *eventManager, int fd) {
    
    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};
    int readStatus = 0;

    ssize_t bytesRead = read_string(fd, readBuffer, ARRAY_SIZE(readBuffer));

    if (bytesRead <= 0) {

        if (!bytesRead || errno == ECONNRESET) {

            trigger_event_client_disconnect(eventManager, fd);
            LOG(INFO, "Client terminated (fd: %d)", fd);
            readStatus = -1;
        }
        else if (bytesRead < 0 && errno != EINTR) {
            trigger_event_client_disconnect(eventManager, fd);
            LOG(ERROR, "Error reading from socket (fd: %d)", fd); 
        }
    }
    else {

        /* the server may receive a partial message 
            from the client due to the nature of the
            TCP protocol. for this reason, all read
            data is stored in the client's buffer, and
            only after the message is received (indicated
            by CRLF), will it be parsed */

        int fdIdx = find_fd_idx_in_hash_table(tcpServer->fdsIdxMap, fd);

        char *inBuffer = get_client_inbuffer(tcpServer->clients[fdIdx]);
        int currentLen = strlen(inBuffer);
        int totalLen = currentLen + strlen(readBuffer);

        int copyBytes = totalLen >= MAX_CHARS + 1 ? MAX_CHARS - currentLen: strlen(readBuffer);
        safe_copy(inBuffer + currentLen, copyBytes + 1, readBuffer);
        currentLen += copyBytes;

        /* IRC messages are terminated with CRLF sequence ("\r\n") */
        if (find_delimiter(inBuffer, CRLF) != NULL) {

            char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
            escape_crlf_sequence(escapedMsg, ARRAY_SIZE(escapedMsg), inBuffer);
            
            LOG(DEBUG, "Received message(s) \"%s\" from client (fd: %d)", escapedMsg, fd);
            readStatus = 1;
        }

        if (!readStatus && currentLen == MAX_CHARS) {
            memset(inBuffer, '\0', MAX_CHARS + 1);
        }
    }

    return readStatus;
}

int server_write(TCPServer *tcpServer, EventManager *eventManager, int fd, const char *message) {

    if (tcpServer == NULL || message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char fmtMessage[MAX_CHARS + 1] = {'\0'};
    int writeStatus = 0;

    /* according to the IRC standard, all valid messages
        should be terminated with CRLF */
    if (!is_terminated(message, CRLF)) {
        terminate_string(fmtMessage, MAX_CHARS + 1, message, CRLF);
        message = fmtMessage;
    }

    ssize_t bytesWritten = write_string(fd, message);

    if (bytesWritten <= 0) {

        if (bytesWritten < 0 && errno == EPIPE) {

            if (!get_int_option_value(OT_THREADS)) {

                trigger_event_client_disconnect(eventManager, fd);
                LOG(INFO, "Client terminated (fd: %d)", fd);
            }
        }
        else if (bytesWritten < 0 && errno != EINTR) {

            if (!get_int_option_value(OT_THREADS)) {

                trigger_event_client_disconnect(eventManager, fd);
                LOG(ERROR, "Error writing to socket (fd: %d)", fd); 
            }
        }
    } 
    else {
        char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
        escape_crlf_sequence(escapedMsg, ARRAY_SIZE(escapedMsg), message);

        LOG(DEBUG, "Sent message \"%s\" to client (fd: %d)", escapedMsg, fd);
        writeStatus = 1;
    }   
    return writeStatus;
}

void trigger_event_client_disconnect(EventManager *eventManager, int fd) {

    Event event = {.eventType = NETWORK_EVENT, .subEventType = NE_CLIENT_DISCONNECT, .dataItem = (DataItem) {.itemInt = fd}, .dataType = INT_TYPE};

    push_event_to_queue(eventManager, &event);
}

int get_server_listen_fd(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->listenFd;
}

void set_server_listen_fd(TCPServer *tcpServer, int listenFd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    tcpServer->listenFd = listenFd;
}