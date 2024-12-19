#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <arpa/inet.h>
#include <stdbool.h>

typedef enum {
    HOSTNAME,
    IP_ADDRESS,
    UNKNOWN_HOST_IDENTIFIER,
    HOST_IDENTIFIER_COUNT
} HostIdentifierType;

/* convert a hostname to an ip address */
bool hostname_to_ip(char *buffer, int size, const char *hostname);

/* convert an ip address to a hostname */
bool ip_to_hostname(char *buffer, int size, const char *ipv4Address);

/* retrieve local address and port */
bool get_local_address(char *buffer, int size, int *port, int fd);

/* retrieve foreign address and port */
bool get_peer_address(char *buffer, int size, int *port, int fd);

/* initialize socket address structure */
void set_sockaddr(struct sockaddr_in *sockaddr, const char *address, int port);

/* check if string is a valid ip address */
bool is_valid_ip(const char *string);

/* check if string is a valid port */
bool is_valid_port(unsigned port);

#endif