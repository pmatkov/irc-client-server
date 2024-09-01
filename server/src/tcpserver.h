#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <poll.h>
#include <arpa/inet.h>

typedef struct Client Client;
typedef struct PollFds PollFds;

int init_server(void);

PollFds * create_pfds(int size);
void delete_pfds(PollFds *pollFds);

struct pollfd * get_pfds(PollFds *pollFds);
int get_allocated_pfds(PollFds *pollFds);

void check_listening_pfd(PollFds *pollFds, int *fdsReady);
void check_connected_pfds(PollFds *pollFds, int *fdsReady, int echoServer);

void set_pfd(PollFds *pollFds, int index, int fd, short events); void unset_pfd(PollFds *pollFds, int index);

#endif