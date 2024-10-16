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

/* getaddrinfo function is used to perform
    DNS lookup. if a hostname is resolved 
    to IP address, the numerical value 
    of the IP address will be converted
    to a string */
int convert_hostname_to_ip_address(char *buffer, int size, const char *hostname) {

    if (buffer == NULL || hostname == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct addrinfo hints, *result, *temp;
    int converted = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        LOG(ERROR, "Error resolving hostname to IP address");
        converted = 0;
    }

    if (converted && result != NULL) {
        temp = result;

        struct sockaddr_in *ipv4 = (struct sockaddr_in *)temp->ai_addr;

        if (inet_ntop(temp->ai_family, &ipv4->sin_addr, buffer, size) == NULL) {
            LOG(ERROR, "Error converting network address structure to string");
            converted = 0;
        }

        freeaddrinfo(result); 
    }

    return converted;
}

/* a string value of the IP address is 
    firstly translated to the numerical form 
    with inet_pton. after that, getnameinfo
    is used to perform reverse DNS lookup 
    to obtain hostname from the IP address */
int convert_ip_to_hostname(char *buffer, int size, const char *ipv4Address) {

    if (buffer == NULL || ipv4Address == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct sockaddr_in sa;
    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;

    int converted = 1;

    if (inet_pton(AF_INET, ipv4Address, &sa.sin_addr) <= 0) {
        LOG(ERROR, "Error translating IP address from string to numerical form");
 
        converted = 0;
    }

    if (converted) {

        if (getnameinfo((struct sockaddr *)&sa, sizeof(struct sockaddr_in), buffer, size, NULL, 0, 0) != 0) {
            LOG(ERROR, "Error resolving IP address to hostname");
            converted = 0;
        }
    }

    return converted;
}

/* in a network client, a TCP socket is bound
    to an IP address by the kernel when the 
    connect call is made. getsockname returns
    numerical value of that IP address. after
    that, this value is translated to string
    with inet_ntop */

int get_local_ip_address(char *buffer, int size, int fd) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct sockaddr_in sa;
    socklen_t saLen = sizeof(sa);

    int converted = 1;

    if (getsockname(fd, (struct sockaddr *)&sa, &saLen) == -1) {
        
        LOG(ERROR, "Error getting local IP address");
        converted = 0;
    }

    if (converted) {
        if (inet_ntop(AF_INET, &sa.sin_addr, buffer, size) <= 0) {

            LOG(ERROR, "Error translating IP address from numerical form to string");
            converted = 0;
        }
    }

    return converted;
}

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

/* for the purpose of this functions, valid ports 
    are those in PORT_MIN - PORT_MAX range */
int is_valid_port(const char *string) {

    long n = str_to_uint(string);

    if (n >= PORT_MIN && n <= PORT_MAX) {
        return 1;
    }
    else {
        return 0;
    }  
}
