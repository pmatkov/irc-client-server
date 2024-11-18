#ifndef CLIENT_H
#define CLIENT_H

#include "../../libs/src/string_utils.h"
#include "../../libs/src/time_utils.h"

#include <arpa/inet.h>

#define MAX_NICKNAME_LEN 9

typedef struct {
    int *fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char inBuffer[MAX_CHARS + 1];
    char ipv4Address[INET_ADDRSTRLEN + 1];
    int port;
    int registered;
    Timer *timer;
} Client;

Client * create_client(void);
void delete_client(Client *client);

int * get_client_fd(Client *client);
void set_client_fd(Client *client, int *fd);
const char * get_client_nickname(Client *client);
void set_client_nickname(Client *client, const char *nickname);
char * get_client_inbuffer(Client *client);
void set_client_inbuffer(Client *client, const char *content);

const char * get_client_ipv4address(Client *client);
void set_client_ipv4address(Client *client, const char *ipv4address);
int get_client_port(Client *client);
void set_client_port(Client *client, int port);

int is_client_registered(Client *client);
void set_client_registered(Client *client, int registered);

#endif