/* --INTERNAL HEADER--
    used for testing */
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "../../libs/src/priv_event.h"
#include "../../libs/src/priv_queue.h"
#include "../../libs/src/session_state.h"
#include "../../libs/src/common.h"
#include "../../libs/src/time_utils.h"
#include "../../libs/src/network_utils.h"

#include <stdbool.h>
#include <poll.h>

typedef struct {
    int fd;
    char serverIdentifier[MAX_CHARS + 1];
    HostIdentifierType identifierType;
    int port;
    char inBuffer[MAX_CHARS + 1];
    Queue *msgQueue;
    Timer *timer;
    SessionStateType clientState;
} TCPClient;

TCPClient * create_client(void);
void delete_client(TCPClient *tcpClient);

int client_connect(TCPClient *tcpClient, EventManager *eventManager, const char *address, int port);
void client_disconnect(TCPClient *tcpClient, EventManager *eventManager);

void terminate_session(TCPClient *tcpClient);

int client_read(TCPClient *tcpClient, EventManager *eventManager);
void client_write(TCPClient *tcpClient, EventManager *eventManager, const char *message);

void enqueue_to_client_queue(TCPClient *tcpClient, void *message);
void * dequeue_from_client_queue(TCPClient *tcpClient);

int get_client_fd(TCPClient *tcpClient);
void set_client_fd(TCPClient *tcpClient, int fd);

const char * get_server_identifier(TCPClient *tcpClient);
void set_server_identifier(TCPClient *tcpClient, const char *serverIdentifier, HostIdentifierType identifierType);

char * get_client_inbuffer(TCPClient *tcpClient);
void set_client_inbuffer(TCPClient *tcpClient, const char *string);
Queue * get_client_queue(TCPClient *tcpClient);

SessionStateType get_client_state_type(TCPClient *tcpClient);
void set_client_state_type(TCPClient *tcpClient, SessionStateType stateType);

bool is_client_connected(TCPClient *tcpClient);

#ifdef TEST

int validate_connection_params(const char *address, int port);
void initialize_session(TCPClient *tcpClient, int fd, const char *serverIdentifier, HostIdentifierType identifierType, int port);

#endif

#endif