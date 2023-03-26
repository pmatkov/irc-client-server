#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <ncursesw/curses.h>

#define SERVER_PORT 50100
#define PORT_LOW 49152
#define PORT_HIGH 65535

void create_conection(WINDOW *, WINDOW *, const char *, const char *);
int is_valid_addr(const char *);
int is_valid_port(const char *);
int is_number(const char *, long *);

#endif