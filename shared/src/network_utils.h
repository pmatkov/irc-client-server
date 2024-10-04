#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <arpa/inet.h>

int convert_hostname_to_ip_address(char *buffer, int size, const char *hostname);
int convert_ip_to_hostname(char *buffer, int size, const char *ipv4Address);
void get_client_ip(char *buffer, int size, int fd);
int is_valid_ip_address(const char *string);
int is_valid_port(const char *string);

#endif