#define _XOPEN_SOURCE 700

#include "network_utils.h"
#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_MIN 49152
#define PORT_MAX 65535
#define IP_ADDRESS_OCTETS 4
#define MAX_BYTE_VALUE 255


/* the translation of a hostname to an IP address is 
    done in two steps. first, DNS lookup is performed 
    to obtain the numerical value of an IP address. 
    second, the received numerical value is converted 
    to the string form */
bool hostname_to_ip(char *buffer, int size, const char *hostname) {

    if (buffer == NULL || hostname == NULL) {
        FAILED(ARG_ERROR, NULL);
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

/* the translation of an IP address to a hostname is 
    done in two steps. first, the address string is
    translated to its numerical value. second, reverse
    DNS lookup is performed to retrieve the hostname */
bool ip_to_hostname(char *buffer, int size, const char *ipv4Address) {

    if (buffer == NULL || ipv4Address == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct sockaddr_in sa;
    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;

    bool converted = 1;

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

/* return local address associated with the socket */
bool get_local_address(char *buffer, int size, int *port, int fd) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);

    bool retrieved = 1;

    if (getsockname(fd, (struct sockaddr *)&sa, &len) == -1) {
        
        LOG(ERROR, "Error getting local IP address");
        retrieved = 0;
    }

    if (retrieved) {
        if (inet_ntop(AF_INET, &sa.sin_addr, buffer, size) <= 0) {

            LOG(ERROR, "Error translating IP address");
            retrieved = 0;
        }
        
        if (retrieved && port != NULL) {
            *port = ntohs(sa.sin_port);
        }
    }

    return retrieved;
}

/* return foreign address and port associated with the socket */
bool get_peer_address(char *buffer, int size, int *port, int fd) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);

    bool retrieved = 1;

    if (getpeername(fd, (struct sockaddr *)&sa, &len) == -1) {
        
        LOG(ERROR, "Error getting local IP address");
        retrieved = 0;
    }

    if (retrieved) {
        if (inet_ntop(AF_INET, &sa.sin_addr, buffer, size) <= 0) {

            LOG(ERROR, "Error translating IP address");
            retrieved = 0;
        }

        if (port != NULL) {
            *port = ntohs(sa.sin_port);
        }
    }

    return retrieved;
}

void set_sockaddr(struct sockaddr_in *sockaddr, const char *address, int port) {

    if (sockaddr == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    memset(sockaddr, 0, sizeof(struct sockaddr_in)); 
    sockaddr->sin_family = AF_INET;

    if (is_valid_port(port)) {
        sockaddr->sin_port = htons(port);
    }

    if (address != NULL) {

        if (!is_valid_ip(address)) {
            hostname_to_ip(ipv4Address, sizeof(ipv4Address), address);
            address = ipv4Address;
        }

        inet_pton(AF_INET, address, &sockaddr->sin_addr);   
    }
    else {
        sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    }
}

bool is_valid_ip(const char *string) {

    bool valid = 0;

    if (string != NULL) {

        int octets = 0;
        char *savePtr;

        char *copy = strdup(string);
        char *token = strtok_r(copy, ".", &savePtr);

        while (token != NULL) {

            long n = str_to_uint(token);

            if (n >= 0 && n <= MAX_BYTE_VALUE) {
                octets++;
            }
            token = strtok_r(NULL, ".", &savePtr);
        }

        free(copy);

        if (octets == IP_ADDRESS_OCTETS && string[strlen(string) - 1] != '.') {
            valid = 1;
        }
    }
    return valid;
}

bool is_valid_port(unsigned port) {

    return port >= PORT_MIN && port <= PORT_MAX;
 
}
