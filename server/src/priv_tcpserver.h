#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "session.h"
#include "../../shared/src/priv_queue.h"

#include <poll.h>
#include <arpa/inet.h>

#define MAX_FDS 1024
#define MAX_CHARS 512
#define MAX_NICKNAME_LEN 9

typedef struct {
    struct pollfd *pfds;
    int capacity;
    int count;
} PollFdSet;

typedef struct {
    char nickname[MAX_NICKNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    char ipv4Address[INET_ADDRSTRLEN + 1];
    int port;
    int registered;
    int fd;
} Client;

typedef struct {
    Client *clients;
    Session *session;
    Queue *msgQueue;   
    char serverName[MAX_CHARS + 1];
    int capacity;
} TCPServer;

PollFdSet * create_pollfd_set(int capacity);
void delete_pollfd_set(PollFdSet *pollFdSet);

int is_pfd_set_empty(PollFdSet *pollFdSet);
int is_pfd_set_full(PollFdSet *pollFdSet);

void set_pfd(PollFdSet *pollFdSet, TCPServer *tcpServer, int index, int fd);
void unset_pfd(PollFdSet *pollFdSet, TCPServer *tcpServer, int index);

void check_listening_pfd(PollFdSet *pollFdSet, TCPServer *tcpServer, int *fdsReady);
void check_connected_pfds(PollFdSet *pollFdSet, TCPServer *tcpServer, int *fdsReady, int echoServer);
void handle_inactive_clients(PollFdSet *pollFdSet, TCPServer *tcpServer);

TCPServer * create_server(const char *serverName, int capacity);
void delete_server(TCPServer *tcpServer);

int init_server(void);

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content);
void add_message_to_server_queue(TCPServer *tcpServer, void *message);

struct pollfd * get_pfds(PollFdSet *pollFdSet);
int get_pfds_capacity(PollFdSet *pollFdSet);

char * get_client_inbuffer(Client *client);
const char * get_client_nickname(Client *client);
void set_client_nickname(Client *client, const char *nickname);
int is_client_registered(Client *client);
void set_client_registered(Client *client, int registered);
int get_client_fd(Client *client);

const char * get_server_name(TCPServer *tcpServer);
Session * get_session(TCPServer *tcpServer);
// int is_client_ready(TCPServer *tcpServer, int fd);
// void add_ready_client(TCPServer *tcpServer, int fd);

#ifdef TEST

Client * create_clients(int capacity);
void delete_clients(Client *clients, int capacity);

int find_pfd_index(PollFdSet *pollFdSet, int fd);
int server_read(PollFdSet *pollFdSet, TCPServer *tcpServer, int i);
void server_write(TCPServer *tcpServer, int i);

#endif

#endif