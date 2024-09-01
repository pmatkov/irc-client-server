#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "session.h"

#include <arpa/inet.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

int connect_to_server(char *address, char *port, Session *session);

#ifdef TEST

STATIC int convert_hostname_to_ip(const char *input, char *result, int len);
STATIC int is_ipv4address(const char *string);
STATIC int is_port(const char *string);

#endif

#endif