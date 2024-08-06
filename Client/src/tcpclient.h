#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

int create_connection(char *address, char *port);

#ifdef TEST
STATIC int is_ipv4address(const char *address);
STATIC int is_port(const char *port);
STATIC int str_to_int(const char *string);
#endif

#endif