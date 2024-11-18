#ifndef TOT_CLIENT_H
#define TOT_CLIENT_H

#include "../../libs/src/queue.h"

#include <poll.h>

#define POLL_FD_COUNT 3
#define PIPE_FD_IDX 2

/* a tcpClient provides networking functionality
    for the app. it tracks the state of the stdin,
    socket and pipe file descriptors */
typedef struct TCPClient TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

/* connect a client to the server at the specified
    hostname or address and port */
int client_connect(TCPClient *tcpClient, const char *address, int port);

/* close tcp connection and remove client from the
    set of poll fd's monitored by poll() */
void client_disconnect(TCPClient *tcpClient); 

/* read data from the tcp socket. returns -1 on 
    server disconnect and error, 1 if full message
    was received and 0 otherwise */
int client_read(TCPClient *tcpClient);

/* write data to the tcp socket */
void client_write(TCPClient *tcpClient, int fd, const char *message);

/* add message to the outbound queue */
void add_message_to_client_queue(TCPClient *tcpClient, void *message);

/* remove message from the outbound queue */
void * remove_message_from_client_queue(TCPClient *tcpClient);

struct pollfd * get_fds(TCPClient *tcpClient);
const char * get_servername(TCPClient *tcpClient);
void set_servername(TCPClient *tcpClient, const char *servername);

Queue * get_client_queue(TCPClient *tcpClient);
char * get_client_inbuffer(TCPClient *tcpClient);
int get_inbuffer_size(TCPClient *tcpClient);

char get_char_from_inbuffer(TCPClient *tcpClient, int index);
void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index);

/* set fd value at the position fdIndex in the
    file descriptors array. the value of -1 removes
    fd from the set of fd's monitored by poll() */
void set_fd(TCPClient *tcpClient, int fdIndex, int fd);

/* remove fd at the position fdIndex from the set
    of fd's monitored by poll() */
void unset_fd(TCPClient *tcpClient, int fdIndex);
int is_client_connected(TCPClient *tcpClient);

/* check if there is input available to read from 
    stdin */
int is_stdin_event(TCPClient *tcpClient);

/* check if there is input available to read from
    tcp socket */
int is_socket_event(TCPClient *tcpClient);

/* check if there is input available to read from 
    the pipe */
int is_pipe_event(TCPClient *tcpClient);

int get_socket_fd(TCPClient *tcpClient);
int get_pipe_fd(TCPClient *tcpClient);
int get_stdin_fd(TCPClient *tcpClient);

#endif