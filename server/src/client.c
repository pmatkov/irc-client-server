#ifdef TEST
#include "priv_client.h"
#else
#include "client.h"
#endif

#include "../../libs/src/time_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#define MAX_FDS 1024         
#define MAX_NICKNAME_LEN 9

#ifndef TEST

struct Client {
    int *fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    char ipv4Address[INET_ADDRSTRLEN + 1];
    int port;
    int registered;
    Timer *timer;
};

#endif

Client * create_client(void) {

    Client *client = (Client *) malloc(sizeof(Client));
    if (client == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");  
    }

    client->fd = NULL;
    memset(client->nickname, '\0', sizeof(client->nickname));
    memset(client->inBuffer, '\0', sizeof(client->inBuffer));
    memset(client->ipv4Address, '\0', sizeof(client->ipv4Address));
    client->port = 0;
    client->registered = 0;
    client->timer = create_timer();

    return client;
}

void delete_client(Client *client) {

    if (client != NULL) {

        delete_timer(client->timer);
    }
    free(client);
}

int * get_client_fd(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->fd;
}

void set_client_fd(Client *client, int *fd) {

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

    safe_copy(client->nickname, sizeof(client->nickname), nickname);
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
    safe_copy(client->inBuffer, sizeof(client->inBuffer), content);
}

const char * get_client_ipv4address(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->ipv4Address;  
}

void set_client_ipv4address(Client *client, const char *ipv4address) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    safe_copy(client->ipv4Address, ARR_SIZE(client->ipv4Address), ipv4address);  
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

int is_client_registered(Client *client) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return client->registered;
}

void set_client_registered(Client *client, int registered) {

    if (client == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    client->registered = registered;
}
