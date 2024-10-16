/* --INTERNAL HEADER--
   used for testing */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "priv_session.h"
#include "../../libs/src/command.h"
#include "../../libs/src/priv_queue.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/time_utils.h"

#include <poll.h>
#include <arpa/inet.h>

#define MAX_FDS 1024
#define MAX_CHARS 512
#define MAX_NICKNAME_LEN 9

typedef struct {
    char nickname[MAX_NICKNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    char ipv4Address[INET_ADDRSTRLEN + 1];
    int port;
    int registered;
    int *fd;
    Timer *timer;
} Client;

typedef struct {
    struct pollfd *pfds;
    Client *clients;
    Session *session;
    Queue *msgQueue;   
    char hostname[MAX_CHARS + 1];
    int capacity;
    int count;
} TCPServer;

TCPServer * create_server(int capacity, const char *hostname);
void delete_server(TCPServer *tcpServer);

int init_server(int port);

int are_pfds_empty(TCPServer *tcpServer);
int are_pfds_full(TCPServer *tcpServer);

void set_fd(TCPServer *tcpServer, int fdIndex, int fd);
void unset_fd(TCPServer *tcpServer, int fdIndex);

int is_fd_ready(TCPServer *tcpServer, int fdIndex);
int find_fd_index(TCPServer *tcpServer, int fd);

void add_client(TCPServer *tcpServer, int listenFdIndex);
void remove_client(TCPServer *tcpServer, int fdIndex);
Client * find_client(TCPServer *tcpServer, const char *nickname);

void remove_inactive_clients(TCPServer *tcpServer, int waitingTime);

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content);
void add_message_to_server_queue(TCPServer *tcpServer, void *message);
void * remove_message_from_server_queue(TCPServer *tcpServer);

const char * get_client_nickname(Client *client);
void set_client_nickname(Client *client, const char *nickname);
char * get_client_inbuffer(Client *client);
void set_client_inbuffer(Client *client, const char *content);
int is_client_registered(Client *client);
void set_client_registered(Client *client, int registered);
int get_client_fd(Client *client);

const char * get_server_name(TCPServer *tcpServer);
Client * get_client(TCPServer *tcpServer, int index);
Session * get_session(TCPServer *tcpServer);
Queue * get_msg_queue(TCPServer *tcpServer);

struct pollfd * get_fds(TCPServer *tcpServer);
int get_fds_capacity(TCPServer *tcpServer);

void send_message_to_user(void *user, void *content);

void send_user_queue_messages(void *user, void *arg);
void send_channel_queue_messages(void *channel, void *session);

int server_read(TCPServer *tcpServer, int fdIndex);
void server_write(const char *message, int fd);

#ifdef TEST

struct pollfd * create_pfds(int capacity);
void delete_pfds(struct pollfd *pfds);
Client * create_clients(int capacity);
void delete_clients(Client *clients, int capacity);

#endif

#endif