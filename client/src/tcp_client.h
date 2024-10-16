#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "../../libs/src/queue.h"

#include <poll.h>

/* a tcpClient provides networking
    functionality for the app. the 
    tcpClient tracks the state of 
    the stdin and tcp socket file 
    descriptors. an input buffer is
    used for incoming messages and
    a queue for outgoing messages */
typedef struct TCPClient TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

/* connect a client to the server at the 
    specified hostname or address and port */
int client_connect(TCPClient *tcpClient, const char *hostOrAddr, const char *port);

/* close tcp connection and remove client
    from the set of poll fd's monitored 
    by poll() */
void client_disconnect(TCPClient *tcpClient); 

/* read data from the tcp socket */
int client_read(TCPClient *tcpClient);

/* write data to the tcp socket */
void client_write(const char *message, int fd);

/* add message to the outbound queue */
void add_message_to_client_queue(TCPClient *tcpClient, void *message);

/* remove message from the outbound queue */
void * remove_message_from_client_queue(TCPClient *tcpClient);

struct pollfd * get_fds(TCPClient *tcpClient);
const char * get_server_name(TCPClient *tcpClient);
void set_server_name(TCPClient *tcpClient, const char *serverName);

Queue * get_client_queue(TCPClient *tcpClient);
char * get_client_inbuffer(TCPClient *tcpClient);

char get_char_from_inbuffer(TCPClient *tcpClient, int index);
void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index);

/* set the value of the file descriptor at
    the position fdIndex in the file descriptors
    array. the value of -1 removes fd from 
    the set of monitored fd's */
void set_fd(TCPClient *tcpClient, int fdIndex, int fd);

/* remove fd at the position fdIndex from the
    set of monitored fd's */
void unset_fd(TCPClient *tcpClient, int fdIndex);
int is_client_connected(TCPClient *tcpClient);

/* check incoming data from stdin (keyboard
    input) */
int is_stdin_event(TCPClient *tcpClient);

/* check incoming data from tcp socket
    (server messages) */
int is_socket_event(TCPClient *tcpClient);

int get_socket_fd(TCPClient *tcpClient);

#endif