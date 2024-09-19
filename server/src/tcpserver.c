#ifdef TEST
#include "priv_tcpserver.h"
#include "../../shared/src/mock.h"
#else
#include "tcpserver.h"
#include "../../shared/src/queue.h"
#endif

#include "command_handler.h"
#include "../../shared/src/command.h"
#include "../../shared/src/priv_message.h"
#include "../../shared/src/time_utils.h"
#include "../../shared/src/string_utils.h"
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
#define MAX_CHARS 512
#define MSG_QUEUE_LEN MAX_FDS/ 100

#define MAX_WAITING_TIME 60

#define SERVER_PORT 50100
#define LISTEN_QUEUE 100

#ifndef TEST

struct PollFdSet {
    struct pollfd *pfds;
    int capacity;
    int count;
};

struct Client {
    char nickname[MAX_NICKNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    char ipv4Address[INET_ADDRSTRLEN + 1];
    int port;
    int registered;
    int fd;
    Timer *timer;
};

struct TCPServer {
    Client *clients;
    Session *session;
    Queue *msgQueue;
    char serverName[MAX_CHARS + 1];
    int capacity;
};

#endif

STATIC Client * create_clients(int capacity);
STATIC void delete_clients(Client *clients, int capacity);

STATIC int find_pfd_index(PollFdSet *pollFdSet, int fd);

STATIC int server_read(PollFdSet *pollFdSet, TCPServer *tcpServer, int i);
STATIC void server_write(TCPServer *tcpServer, int i);

PollFdSet * create_pollfd_set(int capacity) {

    PollFdSet *pollFdSet = (PollFdSet*) malloc(sizeof(PollFdSet));
    if (pollFdSet == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    if (!capacity) {
        capacity = MAX_FDS;
    }

    pollFdSet->pfds = (struct pollfd *) malloc(capacity * sizeof(struct pollfd));
    if (pollFdSet->pfds == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    for (int i = 0; i < MAX_FDS; i++) {
        pollFdSet->pfds[i].fd = -1;
        pollFdSet->pfds[i].events = POLLIN;
    }

    pollFdSet->capacity = capacity;
    pollFdSet->count = 0;

    return pollFdSet;
}

void delete_pollfd_set(PollFdSet *pollFdSet) {

    if (pollFdSet != NULL) {

        free(pollFdSet->pfds);
    }

    free(pollFdSet);
}

int is_pfd_set_empty(PollFdSet *pollFdSet) {

    if (pollFdSet == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return pollFdSet->count == 0;
}

int is_pfd_set_full(PollFdSet *pollFdSet) {

    if (pollFdSet == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return pollFdSet->count == pollFdSet->capacity;
}

void set_pfd(PollFdSet *pollFdSet, TCPServer *tcpServer, int index, int fd) {

    if (pollFdSet == NULL || tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_pfd_set_full(pollFdSet) || index < 0 || index >= pollFdSet->capacity) {
        return;
    }

    pollFdSet->pfds[index].fd = fd;
    tcpServer->clients[index].fd = fd;

    pollFdSet->count++;
}

void unset_pfd(PollFdSet *pollFdSet, TCPServer *tcpServer, int index) {

    if (pollFdSet == NULL || tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (is_pfd_set_empty(pollFdSet) || index < 0 || index >= pollFdSet->capacity) {
        return;
    }

    pollFdSet->pfds[index].fd = -1;
    tcpServer->clients[index].fd = -1;

    pollFdSet->count--;
}

TCPServer * create_server(const char *serverName, int capacity) {

    TCPServer *tcpServer = (TCPServer*) malloc(sizeof(TCPServer));
    if (tcpServer == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    if (!capacity) {
        capacity = MAX_FDS;
    }

    tcpServer->clients = create_clients(capacity);
    tcpServer->session = create_session();
    tcpServer->msgQueue = create_queue(MSG_QUEUE_LEN, sizeof(ExtMessage));
    safe_copy(tcpServer->serverName, MAX_CHARS + 1, serverName);
    tcpServer->capacity = capacity;

    return tcpServer;
}

void delete_server(TCPServer *tcpServer) {

    if (tcpServer != NULL) {

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

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content) {

    if (tcpServer == NULL || client == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strlen(content)) {

        const char *nickname = get_client_nickname(client);

        if (!is_client_registered(client)) {

            ExtMessage *message = create_ext_message("", nickname, content);

            add_message_to_server_queue(tcpServer, message);

            delete_message(message); 
        }
        else {

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

void check_listening_pfd(PollFdSet *pollFdSet, TCPServer *tcpServer, int *fdsReady) {

    if (pollFdSet == NULL || tcpServer == NULL || fdsReady == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (pollFdSet->count && pollFdSet->pfds[0].revents & POLLIN) {

        struct sockaddr_in clientaddr;
        socklen_t clientLen = sizeof(clientaddr);

        // accept connection request on listening socket
        int connectFd = accept(pollFdSet->pfds[0].fd, (struct sockaddr*) &clientaddr, &clientLen);

        if (connectFd < 0) {
            FAILED("Error accepting connection", NO_ERRCODE);
        }

        int index = find_pfd_index(pollFdSet, -1);

        if (index == -1) {
            FAILED("No available pfds", NO_ERRCODE);
        }

        set_pfd(pollFdSet, tcpServer, index, connectFd);

        start_timer(tcpServer->clients[index].timer);

        // get client's IP address and port number
        inet_ntop(AF_INET, &clientaddr.sin_addr, tcpServer->clients[index].ipv4Address, sizeof(tcpServer->clients[index].ipv4Address));
        tcpServer->clients[index].port = ntohs(clientaddr.sin_port); 

        LOG(INFO, "New connection (#%d) from client %s on port %d (fd: %d)", pollFdSet->count - 1, tcpServer->clients[index].ipv4Address, tcpServer->clients[index].port, tcpServer->clients[index].fd);

        (*fdsReady)--;
    }
}

void check_connected_pfds(PollFdSet *pollFdSet, TCPServer *tcpServer, int *fdsReady, int echoServer)  {

    if (pollFdSet == NULL || tcpServer == NULL || fdsReady == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    for (int i = 1; i < pollFdSet->capacity && *fdsReady; i++) {

        // check for events on connected sockets
        if (pollFdSet->pfds[i].revents & (POLLIN | POLLERR)) { 

            int fullMsg = server_read(pollFdSet, tcpServer, i);

            /* if echo server is active and full message 
            was received, send message back to the client */
            if (fullMsg) {
                
                if (echoServer) {
                    server_write(tcpServer, i);
                }
                else {

                    CommandTokens *cmdTokens = create_command_tokens();

                    parse_message(tcpServer, &tcpServer->clients[i], cmdTokens);
                    CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(cmdTokens));

                    if (commandType != UNKNOWN_COMMAND_TYPE) {

                        CommandFunction cmdFunction = get_command_function((CommandType)commandType);
                        cmdFunction(tcpServer, &tcpServer->clients[i], cmdTokens);
                    }

                    memset(tcpServer->clients[i].inBuffer, '\0', MAX_CHARS + 1);

                    delete_command_tokens(cmdTokens);
                }
            }
            (*fdsReady)--;
        }
    }
}

void handle_inactive_clients(PollFdSet *pollFdSet, TCPServer *tcpServer) {

    if (pollFdSet == NULL || tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    for (int i = 1; i < tcpServer->capacity; i++) {

        if (tcpServer->clients[i].fd != -1) {
            if (!is_client_registered(&tcpServer->clients[i])) {

                stop_timer(tcpServer->clients[i].timer);

                if (get_elapsed_time(tcpServer->clients[i].timer) >= MAX_WAITING_TIME) {
                    close(tcpServer->clients[i].fd);
                    unset_pfd(pollFdSet, tcpServer, i);
                    reset_timer(tcpServer->clients[i].timer);
                }
            }
            else {
                if (get_elapsed_time(tcpServer->clients[i].timer)) {
                    reset_timer(tcpServer->clients[i].timer);
                }
            }
        }
    }
}

STATIC Client * create_clients(int capacity) {

    Client *clients = (Client *) malloc(capacity * sizeof(Client));
    if (clients == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);  
    }

    for (int i = 0; i < capacity; i++) {

        memset(clients[i].nickname, '\0', MAX_NICKNAME_LEN + 1);
        memset(clients[i].inBuffer, '\0', MAX_CHARS + 1);
        memset(clients[i].ipv4Address, '\0', INET_ADDRSTRLEN + 1);
        clients[i].port = 0;
        clients[i].registered = 0;
        clients[i].fd = -1;
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

STATIC int find_pfd_index(PollFdSet *pollFdSet, int fd) {

    if (pollFdSet == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    
    int i = 0, found = 0;

    while (i < pollFdSet->capacity && !found) {

        if (pollFdSet->pfds[i].fd == fd) {
            found = 1;
        }
        i++;
    }

    return found ? i : -1;
}

STATIC int server_read(PollFdSet *pollFdSet, TCPServer *tcpServer, int i) {
    
    if (pollFdSet == NULL || tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char readBuffer[MAX_CHARS + 1] = {'\0'};

    ssize_t bytesRead = read(tcpServer->clients[i].fd, readBuffer, MAX_CHARS);

    int fullMsg = 0;

    if (bytesRead <= 0) {

        if (!bytesRead) {
            LOG(INFO, "Client %s on port %d disconnected (fd: %d)", tcpServer->clients[i].ipv4Address, tcpServer->clients[i].port, tcpServer->clients[i].fd);
        }
        else if (errno != ECONNRESET) {
            LOG(ERROR, "Error reading from socket: %d", tcpServer->clients[i].fd);
        }
        close(tcpServer->clients[i].fd);
        unset_pfd(pollFdSet, tcpServer, i);
    }
    else {

        int msgLength = strlen(tcpServer->clients[i].inBuffer);
        int copyBytes = msgLength + bytesRead <= MAX_CHARS ? bytesRead : MAX_CHARS - msgLength;

        strncpy(tcpServer->clients[i].inBuffer + msgLength, readBuffer, copyBytes);
        msgLength += copyBytes;

        // full message received
        if (tcpServer->clients[i].inBuffer[msgLength - 1] == '\n') {

            LOG(INFO, "Message received from fd: %d", tcpServer->clients[i].fd);

            fullMsg = 1;
        }

        if (!fullMsg && msgLength == MAX_CHARS) {
            memset(tcpServer->clients[i].inBuffer, '\0', MAX_CHARS + 1);
        }
    }
    return fullMsg;
}

STATIC void server_write(TCPServer *tcpServer, int i) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    size_t bytesLeft; 
    ssize_t bytesWritten; 
    char *msgPtr = tcpServer->clients[i].inBuffer; 

    bytesLeft = strlen(msgPtr); 

    while (bytesLeft) { 

        if ((bytesWritten = write(tcpServer->clients[i].fd, msgPtr, bytesLeft)) <= 0) { 

            if (bytesWritten < 0 && errno == EINTR) {
                bytesWritten = 0;
            }
            else {
                FAILED("Error writing to socket: %d", NO_ERRCODE, tcpServer->clients[i].fd);

            }
        }
        bytesLeft -= bytesWritten; 
        msgPtr += bytesWritten; 
    }

    // reset input buffer
    tcpServer->clients[i].inBuffer[0] = '\0';
}

struct pollfd * get_pfds(PollFdSet *pollFdSet) {

    return pollFdSet->pfds;
}

int get_pfds_capacity(PollFdSet *pollFdSet) {
    
    return pollFdSet->capacity;
}

char * get_client_inbuffer(Client *client) {

    if (client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return client->inBuffer;
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
    return client->fd;
}

const char * get_server_name(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->serverName;
}

Session * get_session(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return tcpServer->session;
}


// int is_client_ready(TCPServer *tcpServer, int fd) {

//     if (tcpServer == NULL) {
//         FAILED(NULL, ARG_ERROR);
//     }

//     int clientSet = 0;

//     for (int i = 0; i < tcpServer->readyClientsCount && !clientSet; i++) {

//         if (tcpServer->readyClients[i]) {
//             clientSet = 1;
//         }
//     }

//     return clientSet;
// }

// void add_ready_client(TCPServer *tcpServer, int fd) {

//     if (tcpServer == NULL) {
//         FAILED(NULL, ARG_ERROR);
//     }

//     if (!is_client_ready(session, fd)) {
//         tcpServer->readyClients[tcpServer->readyClientsCount] = fd;
//         tcpServer->readyClientsCount++;
//     }
// }

