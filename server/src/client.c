#ifdef TEST
#include "priv_client.h"
#else
#include "client.h"
#include "../../libs/src/common.h"
#endif

#include "../../libs/src/enum_utils.h"

#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct Client {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char clientIdentifier[MAX_CHARS + 1];
    HostIdentifierType identifierType;
    int port;
    char inBuffer[MAX_CHARS + 1];
    SessionStateType stateType;
};

#endif

Client * create_client(void) {

    Client *client = (Client *) malloc(sizeof(Client));
    if (client == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    client->fd = UNASSIGNED;
    memset(client->nickname, '\0', sizeof(client->nickname));
    memset(client->clientIdentifier, '\0', ARRAY_SIZE(client->clientIdentifier));
    client->identifierType = UNKNOWN_HOST_IDENTIFIER;
    client->port = UNASSIGNED;
    memset(client->inBuffer, '\0', sizeof(client->inBuffer));
    client->stateType = DISCONNECTED;

    return client;
}

void delete_client(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    
    free(client);
}

int get_client_fd(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->fd;
}

void set_client_fd(Client *client, int fd) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    client->fd = fd;
}

const char * get_client_nickname(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->nickname;
}

void set_client_nickname(Client *client, const char *nickname) {

    if (client == NULL || nickname == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    safe_copy(client->nickname, ARRAY_SIZE(client->nickname), nickname);
}

const char * get_client_identifier(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return client->clientIdentifier;
}

void set_client_identifier(Client *client, const char *clientIdentifier) {

    if (client == NULL || clientIdentifier == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    safe_copy(client->clientIdentifier, ARRAY_SIZE(client->clientIdentifier), clientIdentifier);
}

HostIdentifierType get_client_identifier_type(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return client->identifierType;
}

void set_client_identifier_type(Client *client, HostIdentifierType identifierType) {

    if (client == NULL || !is_valid_enum_type(identifierType, HOST_IDENTIFIER_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }

    client->identifierType = identifierType;
}

int get_client_port(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->port;  
}

void set_client_port(Client *client, int port) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    client->port = port;  
}

char * get_client_inbuffer(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->inBuffer;
}

void set_client_inbuffer(Client *client, const char *content) {

    if (client == NULL || content == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    safe_copy(client->inBuffer, ARRAY_SIZE(client->inBuffer), content);
}

SessionStateType get_client_state_type(Client *client) {
    
    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->stateType;
}

void set_client_state_type(Client *client, SessionStateType stateType) {
    
    if (client == NULL || !is_valid_enum_type(stateType, SESSION_STATE_TYPE_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }

    client->stateType = stateType;
}

bool is_client_connected(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return client->stateType != DISCONNECTED;
}
