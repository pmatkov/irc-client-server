#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <arpa/inet.h>

/* convert a hostname to ipv4 address */
int convert_hostname_to_ip_address(char *buffer, int size, const char *hostname);

/* convert an ipv4 address to a hostname */
int convert_ip_to_hostname(char *buffer, int size, const char *ipv4Address);

/* retrieve client's local ipv4 address
    (available after connection to the
    server */
int get_local_ip_address(char *buffer, int size, int fd);

/* check if string is a valid ipv4 address */
int is_valid_ip_address(const char *string);

/* checks if string is a valid port */
int is_valid_port(const char *string);

#endif