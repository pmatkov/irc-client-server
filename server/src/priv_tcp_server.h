#define _XOPEN_SOURCE 700
/* --INTERNAL HEADER--
   used for testing */
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "priv_client.h"
#include "priv_session.h"
#include "../../libs/src/priv_event.h"
#include "../../libs/src/priv_queue.h"
#include "../../libs/src/priv_hash_table.h"
#include "../../libs/src/threads.h"
#include "../../libs/src/irc_message.h"
#include "../../libs/src/time_utils.h"

#include <stdbool.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct {
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
} TCPServer;

TCPServer * create_server(int capacity);
void delete_server(TCPServer *tcpServer);

int init_server(TCPServer *tcpServer, const char *address, int port);

bool is_server_empty(TCPServer *tcpServer);
bool is_server_full(TCPServer *tcpServer);

void add_client(TCPServer *tcpServer, EventManager *eventManager);
int accept_connection(TCPServer *tcpServer);
void register_connection(TCPServer *tcpServer, EventManager *eventManager, int fd);
void remove_client(TCPServer *tcpServer, EventManager *eventManager, int fd);
Client * find_client(TCPServer *tcpServer, const char *nickname);

int get_server_capacity(TCPServer *tcpServer);
Client * get_client(TCPServer *tcpServer, int fdIdx);
Session * get_session(TCPServer *tcpServer);
Queue * get_server_out_queue(TCPServer *tcpServer);
HashTable * get_server_fds_idx_map(TCPServer *tcpServer);

void create_server_info(char *buffer, int size, void *arg);

void add_message_to_queue(TCPServer *tcpServer, Client *client, const char *content);
void enqueue_to_server_queue(TCPServer *tcpServer, void *message);
void * dequeue_from_server_queue(TCPServer *tcpServer);
void add_irc_message_to_queue(TCPServer *tcpServer, Client *client, IRCMessage *tokens);

void send_message_to_user(void *user, void *arg);

void send_user_queue_messages(void *user, void *arg);
void send_channel_queue_messages(void *channel, void *arg);

int server_read(TCPServer *tcpServer, EventManager *eventManager, int fd);
int server_write(TCPServer *tcpServer, EventManager *eventManager, int fd, const char *message);

void trigger_event_client_disconnect(EventManager *eventManager, int fd);

int get_server_listen_fd(TCPServer *tcpServer);
void set_server_listen_fd(TCPServer *tcpServer, int listenFd);

#ifdef TEST

int find_client_fd_idx(TCPServer *tcpServer, int fd);
void set_client_data(TCPServer *tcpServer, int fdIdx, int fd, const char *clientIdentifier, HostIdentifierType identifierType, int port);
void unset_client_data(TCPServer *tcpServer, int fdIdx);

#endif

#endif