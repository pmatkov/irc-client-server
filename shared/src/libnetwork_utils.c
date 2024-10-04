#define _XOPEN_SOURCE 700

#include "network_utils.h"
#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_MIN 49152
#define PORT_MAX 65535

int convert_hostname_to_ip_address(char *buffer, int size, const char *hostname) {

    if (buffer == NULL || hostname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct addrinfo hints, *res, *p;
    int converted = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        FAILED("Error converting hostname to IP address", NO_ERRCODE);
    }

    p = res;
    if (p != NULL) {

        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        if (inet_ntop(p->ai_family, &ipv4->sin_addr, buffer, size) != NULL) {
            converted = 1;
        }
    }

    freeaddrinfo(res); 

    return converted;
}

int convert_ip_to_hostname(char *buffer, int size, const char *ipv4Address) {

    if (buffer == NULL || ipv4Address == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct sockaddr_in sa;
    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;

    int converted = 0;

    if (inet_pton(AF_INET, ipv4Address, &sa.sin_addr) <= 0) {
        FAILED("Error converting string to numerical IP address", NO_ERRCODE);
    }

    if (getnameinfo((struct sockaddr *)&sa, sizeof(struct sockaddr_in), buffer, size, NULL, 0, 0) != 0) {
        FAILED("Error resolving IP address to hostname", NO_ERRCODE);
    } else {
        converted = 1;
    }

    return converted;
}

void get_client_ip(char *buffer, int size, int fd) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct sockaddr_in sa;
    socklen_t saLen = sizeof(sa);

    if (getsockname(fd, (struct sockaddr *)&sa, &saLen) == -1) {
        FAILED("Error getting local IP address", NO_ERRCODE);
    }

    if (inet_ntop(AF_INET, &(sa.sin_addr), buffer, size) <= 0) {
        FAILED("Error resolving IP address to hostname", NO_ERRCODE);
    }
}

// checks if string is valid IPv4 address
int is_valid_ip_address(const char *string) {
    
    int octets = 0;
    char *copy, *token, *savePtr;

    copy = strdup(string);
    token = strtok_r(copy, ".", &savePtr);

    while (token) {

        long n = str_to_uint(token);

        if (n >= 0 && n <= 255) {
            octets++;
        }
        else {
            break;
        }
        token = strtok_r(NULL, ".", &savePtr);
    }

    free(copy);

    return octets == 4 && string[strlen(string) - 1] != '.';
}

// checks if string is valid port number (in defined range)
int is_valid_port(const char *string) {

    long n = str_to_uint(string);

    if (n >= PORT_MIN && n <= PORT_MAX) {
        return 1;
    }
    else {
        return 0;
    }  
}
