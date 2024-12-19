#ifndef MOCKS_H
#define MOCKS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ncursesw/curses.h>

/* this library contains a set of functions
    that mock some C library functions, 
    some ncurses functions and some custom 
    functions */
#define read mock_read
#define write mock_write
#define close mock_close
#define socket mock_socket
#define connect mock_connect
#define accept mock_accept
#define get_local_address mock_get_address
#define get_peer_address mock_get_address
#define initscr mock_initscr


/* below functions set and access static variables
    in the mocking library */
int get_mock_fd(void);
void set_mock_fd(int fd);

size_t get_mock_buffer_size(void);
void set_mock_buffer_size(size_t);

void * get_mock_buffer(void);
void set_mock_buffer(void *buffer);

int get_mock_port(void);
void set_mock_port(int port);

struct sockaddr_in * get_mock_sockaddr(void);
void set_mock_sockaddr(struct sockaddr_in *sockaddr);

/* mock functions */
ssize_t mock_read(int fd, void *buffer, size_t readBytes);
ssize_t mock_write(int fd, const void *buffer, size_t writeBytes);
int mock_close(int fd);

int mock_socket(int domain, int type, int protocol);
int mock_connect(int fd, struct sockaddr *servAddr, socklen_t len);
int mock_accept(int fd, struct sockaddr *clientAddr, socklen_t *len);

bool mock_get_address(char *buffer, int size, int *port, int fd);

int mock_get_wheight(WINDOW *window);
int mock_get_wwidth(WINDOW *window);

WINDOW * mock_initscr(void);

SCREEN * create_terminal(void);
void delete_terminal(SCREEN *screen);

void set_initial_fd_count(int count);

#endif