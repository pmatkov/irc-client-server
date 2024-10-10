#ifndef TCPCLIENT_H
#define TCPCLIENT_H

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

/* creates a tcp connection between the 
    client and the server at the specified 
    address and port. if the address is 
    a hostname it will be translated to
    the ipv4 format */
int client_connect(TCPClient *tcpClient, char *hostOrAddr, char *port);

/* reads data from the tcp socket */
int client_read(TCPClient *tcpClient);

/* writes data to the tcp socket */
void client_write(const char *message, int fd);

/* adds message to the outbound queue */
void add_message_to_client_queue(TCPClient *tcpClient, void *message);

/* removes message from the outbound queue */
void * remove_message_from_client_queue(TCPClient *tcpClient);

struct pollfd * get_fds(TCPClient *tcpClient);
const char * get_server_name(TCPClient *tcpClient);
void set_server_name(TCPClient *tcpClient, const char *serverName);
char * get_client_inbuffer(TCPClient *tcpClient);
Queue * get_client_queue(TCPClient *tcpClient);
char get_char_from_inbuffer(TCPClient *tcpClient, int index);
void set_char_in_inbuffer(TCPClient *tcpClient, char ch, int index);

/* sets the value of the file descriptor at
    the position fdIndex in the array of file 
    descriptors monitored by poll(). the value
    of -1 removes fd from the set of fd's 
    monitored by poll() */
void set_fd(TCPClient *tcpClient, int fdIndex, int fd);

/* removes fd at the position fdIndex from the array of fd's 
    monitored by poll(). fd's which are not monitored 
    are assigned the value of -1. */
void unset_fd(TCPClient *tcpClient, int fdIndex);
int is_client_connected(TCPClient *tcpClient);

/* checks for incoming data from sdin
    (keyboard input)*/
int is_stdin_event(TCPClient *tcpClient);

/* checks for incoming data from tcp 
    socket (server messages)*/
int is_socket_event(TCPClient *tcpClient);
int get_socket_fd(TCPClient *tcpClient);

#endif