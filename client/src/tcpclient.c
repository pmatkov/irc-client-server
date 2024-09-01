#define _XOPEN_SOURCE 700

#include "tcpclient.h"
#include "display.h"
#include "../../shared/src/parser.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "50100"
#define PORT_MIN 49152
#define PORT_MAX 65535

#define MAX_CHARS 512

STATIC int convert_hostname_to_ip(const char *input, char *result, int len);
STATIC int is_ipv4address(const char *address);
STATIC int is_port(const char *port);

int connect_to_server(char *address, char *port, Session *session)
{
    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    // set default address and/ or port
    if (address == NULL) {
        address = DEFAULT_ADDRESS;
    }
    if (port == NULL) {
        port = DEFAULT_PORT;
    }

    // convert hostname to ip address
    if (!is_ipv4address(address)) {

        if (!convert_hostname_to_ip(address, ipv4Address, sizeof(ipv4Address))) {
            return -2;
        }
        else {
            address = ipv4Address;
        }
    }
    if (!is_port(port)) {
        return -3;
    }

    // create client's TCP socket
    int clientFd = socket(AF_INET, SOCK_STREAM, 0); 
    if (clientFd < 0) {
        FAILED("Error creating socket", NO_ERRCODE);
    }

    // initialize socket address structure with server's IP address and port
    struct sockaddr_in servaddr;

    memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(str_to_uint(port));

    inet_pton(AF_INET, address, &servaddr.sin_addr);

    // connect to server
    int connStatus = connect(clientFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));

    if (connStatus == 0) {
        
        session_set_fd(session, clientFd);
        session_set_connected(session, 1);
    }
   
    return connStatus;
}

STATIC int convert_hostname_to_ip(const char *input, char *result, int len) {

    if (input == NULL || result == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    struct addrinfo hints, *res, *p;
    int converted = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(input, NULL, &hints, &res) != 0) {
        FAILED("Error converting hostname to IP address", NO_ERRCODE);
    }

    p = res;
    if (p != NULL) {

        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        if (inet_ntop(p->ai_family, &ipv4->sin_addr, result, len) != NULL) {
            converted = 1;
        }
    }

    freeaddrinfo(res); 

    return converted;
}

// checks if string is valid IPv4 address
STATIC int is_ipv4address(const char *string) {
    
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
STATIC int is_port(const char *string) {

    long n = str_to_uint(string);

    if (n >= PORT_MIN && n <= PORT_MAX) {
        return 1;
    }
    else {
        return 0;
    }  
}
