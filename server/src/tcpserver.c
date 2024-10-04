#ifdef TEST
#include "priv_tcpserver.h"
#include "../../shared/src/mock.h"
#else
#include "tcpserver.h"
#include "../../shared/src/time_utils.h"
#endif

#include "../../shared/src/settings.h"
#include "../../shared/src/priv_message.h"
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
#define MSG_QUEUE_LEN MAX_FDS/ 100

#define SERVER_PORT 50100
#define LISTEN_QUEUE 100

#define INT_DIGITS 10

#ifndef TEST

struct Client {
    char nickname[MAX_NICKNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    char ipv4Address[INET_ADDRSTRLEN + 1];
    int port;
    int registered;
    int *fd;
    Timer *timer;
};

struct TCPServer {
    struct pollfd *pfds;
    Client *clients;
    Session *session;
    Queue *msgQueue;
    char serverName[MAX_CHARS + 1];
    int capacity;
    int count;
};

#endif

STATIC struct pollfd * create_pfds(int capacity);
STATIC void delete_pfds(struct pollfd *pfds);
STATIC Client * create_clients(int capacity);
STATIC void delete_clients(Client *clients, int capacity);

TCPServer * create_server(int capacity) {

    TCPServer *tcpServer = (TCPServer*) malloc(sizeof(TCPServer));
    if (tcpServer == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    if (!capacity) {
        capacity = MAX_FDS;
    }

    tcpServer->pfds = create_pfds(capacity);
    tcpServer->clients = create_clients(capacity);
    tcpServer->session = create_session();
    tcpServer->msgQueue = create_queue(MSG_QUEUE_LEN, sizeof(ExtMessage));
    safe_copy(tcpServer->serverName, MAX_CHARS + 1, get_property_value(HOSTNAME));
    tcpServer->capacity = capacity;
    tcpServer->count = 0;

    return tcpServer;
}

void delete_server(TCPServer *tcpServer) {

    if (tcpServer != NULL) {

        delete_pfds(tcpServer->pfds);
        delete_clients(tcpServer->clients, tcpServer->capacity);
        delete_session(tcpServer->session);
        delete_queue(tcpServer->msgQueue);
    }
    free(tcpServer);
}

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

    LOG(INFO, "TCPServer started at %s: %d", inet_ntop(AF_INET, &servaddr.sin_addr, ipv4Address, sizeof(ipv4Address)), ntohs(servaddr.sin_port));

    return listenFd;
}

STATIC struct pollfd * create_pfds(int capacity) {

    if (!capacity) {
        capacity = MAX_FDS;
    }

    struct pollfd *pfds = (struct pollfd *) malloc(capacity * sizeof(struct pollfd));
    if (pfds == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    for (int i = 0; i < capacity; i++) {
        pfds[i].fd = -1;
        pfds[i].events = POLLIN;
    }

    return pfds;
}

STATIC void delete_pfds(struct pollfd *pfds) {

    free(pfds);
}

int are_pfds_empty(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return tcpServer->count == 0;
}

int are_pfds_full(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return tcpServer->count == tcpServer->capacity;
}

void set_fd(TCPServer *tcpServer, int fdIndex, int fd) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (are_pfds_full(tcpServer) || fdIndex < 0 || fdIndex >= tcpServer->capacity) {
        return;
    }

    tcpServer->pfds[fdIndex].fd = fd;
    tcpServer->clients[fdIndex].fd = &tcpServer->pfds[fdIndex].fd;
    tcpServer->count++;

}

void unset_fd(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (are_pfds_empty(tcpServer) || fdIndex < 0 || fdIndex >= tcpServer->capacity) {
        return;
    }

    tcpServer->pfds[fdIndex].fd = -1;
    tcpServer->clients[fdIndex].fd = NULL;
    tcpServer->count--;

}

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content) {

    if (tcpServer == NULL || client == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strlen(content)) {


        if (!is_client_registered(client) && client->fd != NULL) {

            char fdBuffer[INT_DIGITS + 1] = {'\0'};
            uint_to_str(fdBuffer, INT_DIGITS + 1, *client->fd);

            ExtMessage *message = create_ext_message("", fdBuffer, content);
            add_message_to_server_queue(tcpServer, message);
            delete_message(message); 
        }
        else {

            const char *nickname = get_client_nickname(client);
            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

            if (user != NULL) {

                RegMessage *message = create_reg_message(content);
                add_message_to_user_queue(user, message);
                delete_message(message); 
            }
        }
    }
}

void add_message_to_server_queue(TCPServer *tcpServer, void *message) {

    if (tcpServer == NULL || message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    enqueue(tcpServer->msgQueue, message);
}

void * remove_message_from_server_queue(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return dequeue(tcpServer->msgQueue);
}


int is_fd_ready(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int ready = 0;

    if (tcpServer->count && tcpServer->pfds[fdIndex].revents & (POLLIN | POLLERR))  {
        ready = 1;
    }

    return ready;
}

int find_fd_index(TCPServer *tcpServer, int fd) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    
    int i = 0, found = 0;

    while (i < tcpServer->capacity && !found) {

        if (tcpServer->pfds[i].fd == fd) {
            found = 1;
        }
        else {
            i++;
        }
    }
    return found ? i : -1;
}

STATIC Client * create_clients(int capacity) {

    Client *clients = (Client *) malloc(capacity * sizeof(Client));
    if (clients == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    for (int i = 0; i < capacity; i++) {

        memset(clients[i].nickname, '\0', sizeof(clients[i].nickname));
        memset(clients[i].inBuffer, '\0', sizeof(clients[i].inBuffer));
        memset(clients[i].ipv4Address, '\0', sizeof(clients[i].ipv4Address));
        clients[i].port = 0;
        clients[i].registered = 0;
        clients[i].fd = NULL;
        clients[i].timer = create_timer();
    }

    return clients;
}

STATIC void delete_clients(Client *clients, int capacity) {

    if (clients != NULL) {

        for (int i = 0; i < capacity; i++) {
            delete_timer(clients[i].timer);
        }
    }
    free(clients);
}

void add_client(TCPServer *tcpServer, int listenFdIndex) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct sockaddr_in clientaddr;
    socklen_t clientLen = sizeof(clientaddr);

    // accept connection request on listening socket
    int connectFd = accept(tcpServer->pfds[listenFdIndex].fd, (struct sockaddr*) &clientaddr, &clientLen);

    if (connectFd < 0) {
        FAILED("Error accepting connection", NO_ERRCODE);
    }

    int index = find_fd_index(tcpServer, -1);

    if (index == -1) {
        FAILED("No available pfds", NO_ERRCODE);
    }

    set_fd(tcpServer, index, connectFd);

    start_timer(tcpServer->clients[index].timer);

    // get client's IP address and port number
    inet_ntop(AF_INET, &clientaddr.sin_addr, tcpServer->clients[index].ipv4Address, sizeof(tcpServer->clients[index].ipv4Address));
    tcpServer->clients[index].port = ntohs(clientaddr.sin_port); 

    LOG(INFO, "New connection (#%d) from %s:%d (fd: %d)", tcpServer->count - 1, tcpServer->clients[index].ipv4Address, tcpServer->clients[index].port, *tcpServer->clients[index].fd);

}

void remove_client(TCPServer *tcpServer, int fdIndex) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    close(*tcpServer->clients[fdIndex].fd);
    unset_fd(tcpServer, fdIndex);
    memset(tcpServer->clients[fdIndex].nickname, '\0', sizeof(tcpServer->clients[fdIndex].nickname));
}

Client * find_client(TCPServer *tcpServer, const char *nickname) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Client *client = NULL;

    if (nickname != NULL) {

        int found = 0;

        for (int i = 0; i < tcpServer->capacity && !found; i++) {

            if (tcpServer->clients[i].fd != NULL) {

                if (strcmp(tcpServer->clients[i].nickname, nickname) == 0) {
                    client = &tcpServer->clients[i];

                    found = 1;
                }
            }
        }
    }
    return client;
}

void remove_inactive_clients(TCPServer *tcpServer, int waitingTime) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    for (int i = 0, count = 0; i < tcpServer->capacity && count < tcpServer->count; i++) {

        if (tcpServer->clients[i].fd != NULL && is_timer_active(tcpServer->clients[i].timer)) {

            if (!is_client_registered(&tcpServer->clients[i])) {

                stop_timer(tcpServer->clients[i].timer);

                if (get_elapsed_time(tcpServer->clients[i].timer) >= waitingTime) {

                    LOG(INFO, "Inactive client removed (fd: %d) %d", *tcpServer->clients[i].fd);

                    remove_client(tcpServer, i);
                    reset_timer(tcpServer->clients[i].timer);
                }
            }
            else {
                reset_timer(tcpServer->clients[i].timer);
            }
            count++;
        }
    }
}

int server_read(TCPServer *tcpServer, int fdIndex) {
    
    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};

    ssize_t bytesRead = read(*tcpServer->clients[fdIndex].fd, readBuffer, MAX_CHARS);

    int fullMsg = 0;

    if (bytesRead <= 0) {

        if (!bytesRead) {
            LOG(INFO, "Client %s on port %d disconnected (fd: %d)", tcpServer->clients[fdIndex].ipv4Address, tcpServer->clients[fdIndex].port, *tcpServer->clients[fdIndex].fd);
        }
        else if (errno != ECONNRESET) {
            LOG(ERROR, "Error reading from socket: %d", *tcpServer->clients[fdIndex].fd);
        }
        remove_client(tcpServer, fdIndex);
    }
    else {

        int msgLength = strlen(tcpServer->clients[fdIndex].inBuffer);
        int copyBytes = msgLength + bytesRead <= MAX_CHARS ? bytesRead : MAX_CHARS - msgLength;

        strncpy(tcpServer->clients[fdIndex].inBuffer + msgLength, readBuffer, copyBytes);
        msgLength += copyBytes;

        // full message received
        if (is_crlf_terminated(tcpServer->clients[fdIndex].inBuffer)) {

            LOG(INFO, "Message received: \"%s\" (fd: %d)", tcpServer->clients[fdIndex].inBuffer, *tcpServer->clients[fdIndex].fd);

            fullMsg = 1;
        }

        if (!fullMsg && msgLength == MAX_CHARS) {
            memset(tcpServer->clients[fdIndex].inBuffer, '\0', sizeof(tcpServer->clients[fdIndex].inBuffer));
        }
    }
    return fullMsg;
}

void server_write(const char *message, int fd) {

    if (message == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    size_t bytesLeft; 
    ssize_t bytesWritten; 

    const char *msgPtr = message;
    char fmtMessage[MAX_CHARS + 1] = {'\0'};

    if (!is_crlf_terminated(msgPtr)) {

        crlf_terminate(fmtMessage, MAX_CHARS + 1, msgPtr);
        msgPtr = fmtMessage;
    }

    bytesLeft = strlen(msgPtr); 

    while (bytesLeft) { 

        if ((bytesWritten = write(fd, msgPtr, bytesLeft)) <= 0) { 

            if (bytesWritten < 0 && errno == EINTR) {
                bytesWritten = 0;
            }
            else {
                FAILED("Error writing to socket: %d", NO_ERRCODE, fd);
            }
        }
        bytesLeft -= bytesWritten; 
        msgPtr += bytesWritten; 
    }
}

const char * get_client_nickname(Client *client) {

    if (client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return client->nickname;
}

void set_client_nickname(Client *client, const char *nickname) {

    if (client == NULL || nickname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    strcpy(client->nickname, nickname);
}

char * get_client_inbuffer(Client *client) {

    if (client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return client->inBuffer;
}

void set_client_inbuffer(Client *client, const char *content) {

    if (client == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    safe_copy(client->inBuffer, MAX_CHARS + 1, content);
}

int is_client_registered(Client *client) {

    if (client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return client->registered;
}

void set_client_registered(Client *client, int registered) {

    if (client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    client->registered = registered;
}

int get_client_fd(Client *client) {

    if (client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return *client->fd;
}

const char * get_server_name(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->serverName;
}

Client * get_client(TCPServer *tcpServer, int index) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return &tcpServer->clients[index];
}

Session * get_session(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->session;
}

Queue * get_msg_queue(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->msgQueue;
}

struct pollfd * get_fds(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->pfds;
}

int get_fds_capacity(TCPServer *tcpServer) {
    
    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->capacity;
}

void send_message_to_user(void *user, void *content) {

    if (user == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    server_write((const char*) content, (get_user_fd((User*)user)));
}

void send_user_queue_messages(void *user, void *arg) {

    if (user == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Queue *queue = get_user_queue((User*)user);

    while (!is_queue_empty(queue)) {

        RegMessage *message = dequeue(queue);
        char *content = get_reg_message_content(message);

        server_write(content, get_user_fd((User*)user));
    }
}

void send_channel_queue_messages(void *channel, void *session) {

    if (channel == NULL || session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Queue *queue = get_channel_queue((Channel*)channel);

    while (!is_queue_empty(queue)) {

        RegMessage *message = dequeue(queue);
        char *content = get_reg_message_content(message);

        iterate_list(get_users_from_channel_users(find_channel_users((Session*)session, channel)), send_message_to_user, content);

    }

}

