#define _XOPEN_SOURCE 700

#ifdef TEST
#include "priv_tcp_server.h"
#include "../../libs/src/mock.h"
#else
#include "tcp_server.h"
#include "../../libs/src/time_utils.h"
#endif

#include "main.h"

#include "../../libs/src/settings.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

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

// #define MAX_NICKNAME_LEN 9
#define LISTEN_QUEUE 50
#define UNASSIGNED_FD -1

#define CRLF_LEN 2

#ifndef TEST

struct TCPServer {
    struct pollfd *pfds;
    Client **clients;
    Session *session;
    char servername[MAX_CHARS + 1];
    Queue *outQueue;
    int capacity;
    int pfdsCount;
    int clientsCount;
    pthread_rwlock_t fdLock;
    pthread_rwlock_t queueLock;
    pthread_rwlock_t countLock;
};

#endif

STATIC struct pollfd * create_pfds(int capacity);
STATIC void delete_pfds(struct pollfd *pfds);
STATIC void close_fds(TCPServer *tcpServer);

TCPServer * create_server(int capacity, const char *servername) {

    TCPServer *tcpServer = (TCPServer*) malloc(sizeof(TCPServer));
    if (tcpServer == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");  
    }

    if (capacity <= 0 || capacity > MAX_FDS) {
        capacity = MAX_FDS;
    }

    tcpServer->pfds = create_pfds(capacity);

    tcpServer->clients = (Client**) malloc(capacity * sizeof(Client*));
    if (tcpServer->clients == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");  
    }
    for (int i = 0; i < capacity; i++) {
        tcpServer->clients[i] = create_client();
    }

    tcpServer->session = create_session();
    tcpServer->outQueue = create_queue(MAX_FDS/ 100, sizeof(ExtMessage));
    safe_copy(tcpServer->servername, MAX_CHARS + 1, servername);
    tcpServer->capacity = capacity;
    tcpServer->pfdsCount = 0;
    tcpServer->clientsCount = 0;

    pthread_rwlock_init(&tcpServer->fdLock, NULL);
    pthread_rwlock_init(&tcpServer->queueLock, NULL);
    pthread_rwlock_init(&tcpServer->countLock, NULL);

    return tcpServer;
}

void delete_server(TCPServer *tcpServer) {

    if (tcpServer != NULL) {

        close_fds(tcpServer);
        delete_pfds(tcpServer->pfds);

        for (int i = 0; i < tcpServer->capacity; i++) {
            delete_client(tcpServer->clients[i]);
        }
        free(tcpServer->clients);
        delete_session(tcpServer->session);
        delete_queue(tcpServer->outQueue);

        pthread_rwlock_destroy(&tcpServer->fdLock);
        pthread_rwlock_destroy(&tcpServer->queueLock);
        pthread_rwlock_destroy(&tcpServer->countLock);
    }
    free(tcpServer);
}

int init_server(const char *address, int port) {

    /* create servers' listening TCP socket */
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        FAILED(NO_ERRCODE, "Error creating socket");
    }

    /* allow socket to bind to the same address/ port 
        (enables quick restart without waiting for 2x
        MSL) */
    int enableSockOpt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &enableSockOpt, sizeof(int)) < 0) {
        FAILED(NO_ERRCODE, "Error setting socket option");
    }

    /* initialize socket address structure */
    struct sockaddr_in servaddr;
    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = is_valid_port(port) ? htons(port) : 0;

    /* INADDR_ANY allows server to accept connection 
        requests on any network interface */
    if (address == NULL) {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        if (!is_valid_ip(address) && hostname_to_ip(ipv4Address, sizeof(ipv4Address), address)) {
            address = ipv4Address;
        }
        inet_pton(AF_INET, address, &servaddr.sin_addr);
    }

    /* bind socket to an address and a port */
    if (bind(listenFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in)) < 0) {
        FAILED(NO_ERRCODE, "Error binding address to socket");
    }

    /* designate socket as a listening socket */
    if (listen(listenFd, LISTEN_QUEUE) < 0) {
        FAILED(NO_ERRCODE, "Error setting listening socket");  
    }

    LOG(INFO, "Waiting for connection at %s: %d", address, port);

    return listenFd;
}

STATIC struct pollfd * create_pfds(int capacity) {

    if (capacity <= 0 || capacity > MAX_FDS) {
        capacity = MAX_FDS;
    }

    struct pollfd *pfds = (struct pollfd *) malloc(capacity * sizeof(struct pollfd));
    if (pfds == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");  
    }

    for (int i = 0; i < capacity; i++) {
        pfds[i].fd = UNASSIGNED_FD;
        pfds[i].events = POLLIN;
    }

    return pfds;
}

STATIC void delete_pfds(struct pollfd *pfds) {

    free(pfds);
}

STATIC void close_fds(TCPServer *tcpServer) {

     if (tcpServer != NULL) {
        for (int i = 0; i < tcpServer->capacity && tcpServer->pfdsCount; i++) {

            if (tcpServer->pfds[i].fd != UNASSIGNED_FD) {
                close(tcpServer->pfds[i].fd);
            }
        }
     }
}

int are_pfds_empty(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->countLock);
    }
    count = tcpServer->pfdsCount;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
    }

    return count == 0;
}

int are_pfds_full(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->countLock);
    }
    count = tcpServer->pfdsCount;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
    }
    return count == tcpServer->capacity;
}


void assign_fd(TCPServer *tcpServer, int fdIndex, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (fdIndex < 0 || fdIndex >= tcpServer->capacity) {
        LOG(ERROR, "Unable to set fd. Invalid index");
        return;
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&tcpServer->fdLock);
        pthread_rwlock_wrlock(&tcpServer->countLock);
    }
    if (tcpServer->pfds[fdIndex].fd == UNASSIGNED_FD) {
        tcpServer->pfds[fdIndex].fd = fd;
        tcpServer->pfdsCount++;

    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }
}

void assign_client_fd(TCPServer *tcpServer, int fdIndex, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (fdIndex < 0 || fdIndex >= tcpServer->capacity) {
        LOG(ERROR, "Unable to set fd. Invalid index");
        return;
    }

    if (get_int_option_value(OT_THREADS)) {

        pthread_rwlock_wrlock(&tcpServer->fdLock);
        pthread_rwlock_wrlock(&tcpServer->countLock);
    }
    if (tcpServer->pfds[fdIndex].fd != UNASSIGNED_FD) {
        set_client_fd(tcpServer->clients[fdIndex], &tcpServer->pfds[fdIndex].fd);
        tcpServer->clientsCount++;
    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }
}

void unassign_fd(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (fdIndex < 0 || fdIndex >= tcpServer->capacity) {
        LOG(ERROR, "Unable to clear fd. Invalid index");
        return;
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&tcpServer->fdLock);
        pthread_rwlock_wrlock(&tcpServer->countLock);
    }
    if (tcpServer->pfds[fdIndex].fd != UNASSIGNED_FD) {
        tcpServer->pfds[fdIndex].fd = UNASSIGNED_FD;
        tcpServer->pfdsCount--;
    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }
}

void unassign_client_fd(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (fdIndex < 0 || fdIndex >= tcpServer->capacity) {
        LOG(ERROR, "Unable to clear fd. Invalid index");
        return;
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_wrlock(&tcpServer->fdLock);
        pthread_rwlock_wrlock(&tcpServer->countLock);
    }
    if (get_client_fd(tcpServer->clients[fdIndex]) != NULL) {
        set_client_fd(tcpServer->clients[fdIndex], NULL);
        tcpServer->clientsCount--;
    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }
}

int is_fd_ready(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return tcpServer->pfds[fdIndex].revents & POLLIN;
}

int find_fd_index(TCPServer *tcpServer, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    
    int i = 0, found = 0;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->fdLock);
    }
    for (; i < tcpServer->capacity && !found; i++) {

        if (tcpServer->pfds[i].fd == fd) {
            found = 1;
        }
    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }

    return found ? i - 1 : -1;
}

void add_client(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (are_pfds_full(tcpServer)) {
        LOG(WARNING, "Unable to add more clients. Max capacity reached");
        return;
    }

    int connectFd = accept_connection(tcpServer);
    int fdIndex = find_fd_index(tcpServer, -1);

    register_connection(tcpServer, fdIndex, connectFd);
}

int accept_connection(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct sockaddr_in clientaddr;
    socklen_t clientLen = sizeof(clientaddr);

    int connectFd = accept(tcpServer->pfds[LISTEN_FD_IDX].fd, (struct sockaddr*) &clientaddr, &clientLen);

    if (connectFd < 0) {
        FAILED(NO_ERRCODE, "Error accepting connection");
    }
    return connectFd;
}   

void register_connection(TCPServer *tcpServer, int fdIndex, int fd) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    assign_fd(tcpServer, fdIndex, fd);
    assign_client_fd(tcpServer, fdIndex, fd);

    // start_timer(tcpServer->clients[fdIndex].timer);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->countLock);
    }

    char ipv4Address[INET_ADDRSTRLEN + 1] = {'\0'};
    int port;


    get_peer_address(ipv4Address, INET_ADDRSTRLEN, &port, fd);

    set_client_ipv4address(tcpServer->clients[fdIndex], ipv4Address);
    set_client_port(tcpServer->clients[fdIndex], port);

    LOG(INFO, "New client connection (#%d) from %s: %d (fd: %d)", tcpServer->clientsCount, ipv4Address, port, fd);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->countLock);
    }
}

void remove_client(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->fdLock);
    }

    int fd = *get_client_fd(tcpServer->clients[fdIndex]);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }

    close(fd);

    unassign_fd(tcpServer, fdIndex);
    unassign_client_fd(tcpServer, fdIndex);

    set_client_registered(tcpServer->clients[fdIndex], 0);
    
    LOG(DEBUG, "Connection closed (fd: %d)", fd);

}

Client * find_client(TCPServer *tcpServer, const char *nickname) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Client *client = NULL;

    if (nickname != NULL) {

        int found = 0;

        if (get_int_option_value(OT_THREADS)) {
            pthread_rwlock_rdlock(&tcpServer->fdLock);
        }
        for (int i = 0; i < tcpServer->capacity && !found; i++) {

            if (get_client_fd(tcpServer->clients[i]) != NULL) {

                if (strcmp(get_client_nickname(tcpServer->clients[i]), nickname) == 0) {
                    client = tcpServer->clients[i];
                    found = 1;
                }
            }
        }
        if (get_int_option_value(OT_THREADS)) {
            pthread_rwlock_unlock(&tcpServer->fdLock);
        }
    }
    return client;
}

// void remove_unregistered_clients(TCPServer *tcpServer, int waitingTime) {

//     if (tcpServer == NULL) {
//         FAILED(ARG_ERROR, NULL);
//     }

//     for (int i = 0, count = 0; i < tcpServer->capacity && count < tcpServer->count; i++) {

//         if (tcpServer->clients[i].fd != NULL && is_timer_active(tcpServer->clients[i].timer)) {

//             if (!is_client_registered(&tcpServer->clients[i])) {

//                 stop_timer(tcpServer->clients[i].timer);

//                 if (get_elapsed_time(tcpServer->clients[i].timer, SECONDS) >= waitingTime) {

//                     LOG(INFO, "Unregistered client %s:%d (fd: %d) removed", tcpServer->clients[i].ipv4Address, tcpServer->clients[i].port, *tcpServer->clients[i].fd);

//                     remove_client(tcpServer, i);
//                     reset_timer(tcpServer->clients[i].timer);
//                 }
//             }
//             else {
//                 reset_timer(tcpServer->clients[i].timer);
//             }
//             count++;
//         }
//     }
// }

int server_read(TCPServer *tcpServer, int fdIndex) {
    
    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};
    int readStatus = 0;

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->fdLock);
    }

    int fd = *get_client_fd(tcpServer->clients[fdIndex]);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }
    ssize_t bytesRead = read_string(fd, readBuffer, ARR_SIZE(readBuffer) - 1);

    if (bytesRead <= 0) {

        if (!bytesRead || errno == ECONNRESET) {
            remove_client(tcpServer, fdIndex);
            LOG(INFO, "Client disconnected (fd: %d)", fd); 
        }
        else if (bytesRead < 0 && errno != EINTR) {
            remove_client(tcpServer, fdIndex);
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

        char *inBuffer = get_client_inbuffer(tcpServer->clients[fdIndex]);
        int inBufferLen = strlen(inBuffer);

        int copyBytes = inBufferLen + strlen(readBuffer) > MAX_CHARS ? MAX_CHARS - inBufferLen : strlen(readBuffer);
        strncpy(inBuffer + inBufferLen, readBuffer, copyBytes);
        inBufferLen += copyBytes;

        /* IRC messages are terminated with CRLF sequence ("\r\n") */
        if (find_delimiter(inBuffer, CRLF) != NULL) {

            char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
            escape_crlf_sequence(escapedMsg, sizeof(escapedMsg), inBuffer);
            
            LOG(DEBUG, "Received message(s) \"%s\" from socket (fd: %d)", escapedMsg, fd);
            readStatus = 1;
        }

        if (!readStatus && inBufferLen == MAX_CHARS) {
            memset(inBuffer, '\0', MAX_CHARS + 1);
        }
    }

    return readStatus;
}

int server_write(TCPServer *tcpServer, int fdIndex, const char *message) {

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

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->fdLock);
    }

    int fd = *get_client_fd(tcpServer->clients[fdIndex]);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }

    ssize_t bytesWritten = write_string(fd, message);

    if (bytesWritten <= 0) {

        if (bytesWritten < 0 && errno == EPIPE) {

            if (!get_int_option_value(OT_THREADS)) {
                remove_client(tcpServer, fdIndex);
                LOG(INFO, "Client disconnected (fd: %d)", fd); 
            }
        }
        else if (bytesWritten < 0 && errno != EINTR) {

            if (!get_int_option_value(OT_THREADS)) {
                remove_client(tcpServer, fdIndex);
                LOG(ERROR, "Error writing to socket (fd: %d)", fd); 
            }
        }
    } 
    else {
        char escapedMsg[MAX_CHARS + sizeof(CRLF) + 1] = {'\0'};
        escape_crlf_sequence(escapedMsg, ARR_SIZE(escapedMsg), message);

        LOG(DEBUG, "Sent message \"%s\" to socket (fd: %d)", escapedMsg, fd);
        writeStatus = 1;
    }   
    return writeStatus;
}

struct pollfd * get_fds(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->pfds;
}

int get_fds_capacity(TCPServer *tcpServer) {
    
    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->capacity;
}

Client * get_client(TCPServer *tcpServer, int index) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->clients[index];
}

Session * get_session(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->session;
}

const char * get_servername(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->servername;
}

Queue * get_server_out_queue(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return tcpServer->outQueue;
}

void create_server_info(char *buffer, int size, void *tcpServer) {

    if (buffer == NULL || tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const char *servername = ((TCPServer*)tcpServer)->servername;

    if (strlen(servername) <= size) {
        safe_copy(buffer, size, servername);
    }
    else {
        LOG(ERROR, "Max message length exceeded");
    }
}

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content) {

    if (tcpServer == NULL || client == NULL || content == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_rdlock(&tcpServer->fdLock);
    }
    int fd = *get_client_fd(client);

    if (get_int_option_value(OT_THREADS)) {
        pthread_rwlock_unlock(&tcpServer->fdLock);
    }

    if (fd != UNASSIGNED_FD && !is_client_registered(client)) {

        char fdStr[MAX_DIGITS + 1] = {'\0'};
        uint_to_str(fdStr, ARR_SIZE(fdStr), fd);

        ExtMessage *message = create_ext_message("", fdStr, content);
        enqueue_to_server_queue(tcpServer, message);
        delete_message(message); 
    }
    else if (is_client_registered(client)) {

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
        int fdIndex;
        const char *message;
    } *data = arg;

    server_write(data->tcpServer, data->fdIndex, data->message);
}

void send_user_queue_messages(void *user, void *arg) {

    if (user == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        TCPServer *tcpServer;
        int fdIndex;
        const char *message;
    } *data = arg;

    while ((data->message = dequeue_from_user_queue((User*)user)) != NULL) {

        server_write(data->tcpServer, data->fdIndex, data->message);
    }
}

void send_channel_queue_messages(void *channel, void *arg) {

    if (channel == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        TCPServer *tcpServer;
        int fdIndex;
        const char *message;
    } *data = arg;

    while ((data->message = dequeue_from_channel_queue((Channel*)channel)) != NULL) {

        Session *session = get_session(data->tcpServer);

        iterate_list(get_users_from_channel_users(find_channel_users(session, channel)), send_message_to_user, data);

    }
}