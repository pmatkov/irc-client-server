#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "../../shared/src/queue.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "50100"

typedef struct TCPClient TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

int connect_to_server(TCPClient *tcpClient, char *address, char *port);

int client_read(TCPClient *tcpClient);
void client_write(TCPClient *tcpClient);

const char * client_get_servername(TCPClient *tcpClient);
void client_set_servername(TCPClient *tcpClient, const char *servername);
const char * client_get_channelname(TCPClient *tcpClient);
void client_set_channelname(TCPClient *tcpClient, const char *channelname);

Queue * client_get_queue(TCPClient *tcpClient);
char * client_get_buffer(TCPClient *tcpClient);
char client_get_char_in_buffer(TCPClient *tcpClient, int index);
void client_set_char_in_buffer(TCPClient *tcpClient, char ch, int index);
int client_get_fd(TCPClient *tcpClient);
void client_set_fd(TCPClient *tcpClient, int fd);
int client_is_connected(TCPClient *tcpClient);
void client_set_connected(TCPClient *tcpClient, int connected);
int client_is_inchannel(TCPClient *tcpClient);
void client_set_inchannel(TCPClient *tcpClient, int inChannel);

void add_message_to_client_queue(TCPClient *tcpClient, void *message);
void * remove_message_from_client_queue(Queue *queue);

#endif