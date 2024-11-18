#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "session.h"
#include "client.h"
#include "../../libs/src/irc_message.h"
#include "../../libs/src/queue.h"
#include "../../libs/src/string_utils.h"

#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>

#define MAX_FDS 1024
#define LISTEN_FD_IDX 0
#define PIPE_FD_IDX 1

// typedef struct Client Client;
typedef struct TCPServer TCPServer;

TCPServer * create_server(int capacity, const char *servername);
void delete_server(TCPServer *tcpServer);

int init_server(const char *address, int port);

int are_pfds_empty(TCPServer *tcpServer);
int are_pfds_full(TCPServer *tcpServer);

void assign_fd(TCPServer *tcpServer, int fdIndex, int fd);
void assign_client_fd(TCPServer *tcpServer, int fdIndex, int fd);

void unassign_fd(TCPServer *tcpServer, int fdIndex);
void unassign_client_fd(TCPServer *tcpServer, int fdIndex);

int is_fd_ready(TCPServer *tcpServer, int fdIndex);
int find_fd_index(TCPServer *tcpServer, int fd);

void add_client(TCPServer *tcpServer);
int accept_connection(TCPServer *tcpServer);
void register_connection(TCPServer *tcpServer, int fdIndex, int fd);
void remove_client(TCPServer *tcpServer, int fdIndex);
Client * find_client(TCPServer *tcpServer, const char *nickname);

// void remove_unregistered_clients(TCPServer *tcpServer, int waitingTime);

// const char * get_client_nickname(Client *client);
// void set_client_nickname(Client *client, const char *nickname);
// char * get_client_inbuffer(Client *client);
// void set_client_inbuffer(Client *client, const char *content);
// const char * get_client_ipv4address(Client *client);

// int is_client_registered(Client *client);
// void set_client_registered(Client *client, int registered);
// int get_client_fd(TCPServer *tcpServer, Client *client);

struct pollfd * get_fds(TCPServer *tcpServer);
int get_fds_capacity(TCPServer *tcpServer);
Client * get_client(TCPServer *tcpServer, int index);
Session * get_session(TCPServer *tcpServer);
const char * get_servername(TCPServer *tcpServer);
Queue * get_server_out_queue(TCPServer *tcpServer);

void create_server_info(char *buffer, int size, void *tcpServer);

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content);
void enqueue_to_server_queue(TCPServer *tcpServer, void *message);
void * dequeue_from_server_queue(TCPServer *tcpServer);
void add_irc_message_to_queue(TCPServer *tcpServer, Client *client, IRCMessage *tokens);

void send_message_to_user(void *user, void *arg);

void send_user_queue_messages(void *user, void *arg);
void send_channel_queue_messages(void *channel, void *arg);

int server_read(TCPServer *tcpServer, int fdIndex);
int server_write(TCPServer *tcpServer, int fdIndex, const char *message);

#endif