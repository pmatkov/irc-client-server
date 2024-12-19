#ifndef CLIENT_H
#define CLIENT_H

#include "../../libs/src/session_state.h"
#include "../../libs/src/network_utils.h"

#include <stdbool.h>

typedef struct Client Client;

Client * create_client(void);
void delete_client(Client *client);

int get_client_fd(Client *client);
void set_client_fd(Client *client, int fd);

const char * get_client_nickname(Client *client);
void set_client_nickname(Client *client, const char *nickname);

const char * get_client_identifier(Client *client);
void set_client_identifier(Client *client, const char *clientIdentifier);

HostIdentifierType get_client_identifier_type(Client *client);
void set_client_identifier_type(Client *client, HostIdentifierType identifierType);

int get_client_port(Client *client);
void set_client_port(Client *client, int port);

char * get_client_inbuffer(Client *client);
void set_client_inbuffer(Client *client, const char *content);

SessionStateType get_client_state_type(Client *client);
void set_client_state_type(Client *client, SessionStateType stateType);

bool is_client_connected(Client *client);


#endif