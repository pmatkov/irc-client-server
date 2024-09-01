#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <poll.h>
#include <arpa/inet.h>

// max message length 512B (including LF)
#define MAX_MSG_LEN 512
#define MAX_NICKNAME_LEN 9

typedef struct {
    int fd;
    char nickname[MAX_NICKNAME_LEN + 1];
    char ipv4Address[INET_ADDRSTRLEN];
    int port;
    char msgBuffer[MAX_MSG_LEN + 1];
} Client;

typedef struct {
    struct pollfd *pfds;
    Client *clients;
    int allocatedPfds;
    int usedPfds;
} PollFds;

int init_server(void);

PollFds * create_pfds(int size);
void delete_pfds(PollFds *pollFds);

struct pollfd * get_pfds(PollFds *pollFds);
int get_allocated_pfds(PollFds *pollFds);

void check_listening_pfd(PollFds *pollFds, int *fdsReady);
void check_connected_pfds(PollFds *pollFds, int *fdsReady, int echoServer);

void set_pfd(PollFds *pollFds, int index, int fd, short events); void unset_pfd(PollFds *pollFds, int index);

#ifdef TEST

Client * create_clients(int size);
void delete_clients(Client *clients);

int find_pfd_index(PollFds *pollFds, int fd);
int read_data(PollFds *pollFds, int i);
void write_data(PollFds *pollFds, int i);

#endif

#endif