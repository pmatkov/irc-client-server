#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "../../libs/src/event.h"
#include "../../libs/src/queue.h"
#include "../../libs/src/session_state.h"
#include "../../libs/src/network_utils.h"

#include <stdbool.h>

/* TCPClient provides networking functionality for the app */
typedef struct TCPClient TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

/* connect to the server at the specified address and port */
int client_connect(TCPClient *tcpClient, EventManager *eventManager, const char *address, int port);

/* close the connection to the server */
void client_disconnect(TCPClient *tcpClient, EventManager *eventManager);

void terminate_session(TCPClient *tcpClient);

/* read data from the socket */
int client_read(TCPClient *tcpClient, EventManager *eventManager);

/* write data to the socket */
void client_write(TCPClient *tcpClient, EventManager *eventManager, const char *message);

/* add a message to the outbound queue */
void enqueue_to_client_queue(TCPClient *tcpClient, void *message);

/* remove a message from the outbound queue */
void * dequeue_from_client_queue(TCPClient *tcpClient);

/* get the file descriptor of the client socket */
int get_client_fd(TCPClient *tcpClient);

/* set the file descriptor of the client socket */
void set_client_fd(TCPClient *tcpClient, int fd);

const char * get_server_identifier(TCPClient *tcpClient);
void set_server_identifier(TCPClient *tcpClient, const char *serverIdentifier, HostIdentifierType identifierType);

char * get_client_inbuffer(TCPClient *tcpClient);
void set_client_inbuffer(TCPClient *tcpClient, const char *string);


Queue * get_client_queue(TCPClient *tcpClient);

SessionStateType get_client_state_type(TCPClient *tcpClient);
void set_client_state_type(TCPClient *tcpClient, SessionStateType stateType);

bool is_client_connected(TCPClient *tcpClient);

#endif