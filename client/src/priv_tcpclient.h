#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "../../shared/src/priv_queue.h"
#include "../../shared/src/string_utils.h"

#include <poll.h>

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "50100"

#define FD_COUNT 2

typedef struct {
    struct pollfd pfds[FD_COUNT];
    char inBuffer[MAX_CHARS + 1];
    Queue *msgQueue;
    char serverName[MAX_CHARS + 1];
} TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

int client_connect(TCPClient *tcpClient, char *address, char *port);

int client_read(TCPClient *tcpClient);
void client_write(const char *message, int fd);

void add_message_to_client_queue(TCPClient *tcpClient, void *message);
void * remove_message_from_client_queue(TCPClient *tcpClient);

struct pollfd * get_fds(TCPClient *tcpClient);
const char * get_server_name(TCPClient *tcpClient);
void set_server_name(TCPClient *tcpClient, const char *serverName);
char * get_client_inbuffer(TCPClient *tcpClient);
char get_char_from_inbuffer(TCPClient *tcpClient, int index);
void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index);
Queue * get_client_queue(TCPClient *tcpClient);
void set_fd(TCPClient *tcpClient, int fdIndex, int fd);
void unset_fd(TCPClient *tcpClient, int fdIndex);
int is_client_connected(TCPClient *tcpClient);
int is_stdin_event(TCPClient *tcpClient);
int is_socket_event(TCPClient *tcpClient);
int get_socket_fd(TCPClient *tcpClient);

#endif