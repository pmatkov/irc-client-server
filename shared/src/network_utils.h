#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <arpa/inet.h>

int convert_hostname_to_ip(const char *input, char *result, int size);
int convert_ip_to_hostname(const char *input, char *result, int size);
void get_localhost_ip(char *result, int size, int fd);
int is_ipv4address(const char *string);
int is_port(const char *string);

#endif