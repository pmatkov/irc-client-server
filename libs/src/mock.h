#ifndef MOCKS_H
#define MOCKS_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ncursesw/curses.h>

#define read mock_read
#define write mock_write
#define close mock_close
#define socket mock_socket
#define connect mock_connect
#define accept mock_accept
#define get_client_ip mock_get_client_ip
#define initscr mock_initscr

#ifdef get_wwidth
#undef get_wwidth
#define get_wwidth(win) 50 
#endif

#ifdef get_wheight
#undef get_wheight
#define get_wheight(win) 50 
#endif

int get_mock_fd(void);
void set_mock_fd(int fd);

size_t get_mock_len(void);
void set_mock_len(size_t);

void * get_mock_buffer(void);
void set_mock_buffer(void *buffer);

int get_mock_port(void);
void set_mock_port(int port);

struct sockaddr_in * get_sockaddr(void);
void set_sockaddr(struct sockaddr_in *sa);

WINDOW * get_mock_stdscr(void);
void set_mock_stdscr(WINDOW *stdscr);

ssize_t mock_read(int fd, void *buffer, size_t len);
ssize_t mock_write(int fd, const void *buffer, size_t len);
int mock_close(int fd);

int mock_socket(int domain, int type, int protocol);
int mock_connect(int fd, struct sockaddr *servAddr, socklen_t len);
int mock_accept(int fd, struct sockaddr *clientAddr, socklen_t *len);

void mock_get_client_ip(char *buffer, int size, int fd);

WINDOW * mock_initscr(void);

SCREEN * create_terminal(void);
void delete_terminal(SCREEN *screen);
void set_initial_fd_count(int count);

#endif