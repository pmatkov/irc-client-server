/* --INTERNAL HEADER--
    used for testing */

#ifndef TOT_CLIENT_H
#define TOT_CLIENT_H

#include "../../libs/src/priv_queue.h"
#include "../../libs/src/string_utils.h"

#include <poll.h>

#define POLL_FD_COUNT 3
#define PIPE_FD_IDX 2

typedef struct {
    struct pollfd pfds[POLL_FD_COUNT];
    char inBuffer[MAX_CHARS + 1];
    Queue *msgQueue;
    char servername[MAX_CHARS + 1];
} TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

int client_connect(TCPClient *tcpClient, const char *address, int port);
void client_disconnect(TCPClient *tcpClient);

int client_read(TCPClient *tcpClient);
void client_write(TCPClient *tcpClient, int fd, const char *message);

void add_message_to_client_queue(TCPClient *tcpClient, void *message);
void * remove_message_from_client_queue(TCPClient *tcpClient);

struct pollfd * get_fds(TCPClient *tcpClient);
const char * get_servername(TCPClient *tcpClient);
void set_servername(TCPClient *tcpClient, const char *servername);

Queue * get_client_queue(TCPClient *tcpClient);
char * get_client_inbuffer(TCPClient *tcpClient);
int get_inbuffer_size(TCPClient *tcpClient);

char get_char_from_inbuffer(TCPClient *tcpClient, int index);
void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index);

void set_fd(TCPClient *tcpClient, int fdIndex, int fd);
void unset_fd(TCPClient *tcpClient, int fdIndex);

int is_client_connected(TCPClient *tcpClient);

int is_stdin_event(TCPClient *tcpClient);
int is_socket_event(TCPClient *tcpClient);
int is_pipe_event(TCPClient *tcpClient);

int get_socket_fd(TCPClient *tcpClient);
int get_pipe_fd(TCPClient *tcpClient);
int get_stdin_fd(TCPClient *tcpClient);

#ifdef TEST

void register_connection(TCPClient *tcpClient, int clientFd, const char *address, int port);

#endif

#endif