#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <arpa/inet.h>

/* convert a hostname to an ip address */
int hostname_to_ip(char *buffer, int size, const char *hostname);

/* convert an ip address to a hostname */
int ip_to_hostname(char *buffer, int size, const char *ipv4Address);

/* retrieve local address and port */
int get_local_address(char *buffer, int size, int *port, int fd);

/* retrieve foreign address and port */
int get_peer_address(char *buffer, int size, int *port, int fd);

/* check if string is a valid ip address */
int is_valid_ip(const char *string);

/* check if string is a valid port */
int is_valid_port(unsigned port);

#endif