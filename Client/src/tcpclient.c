#include "tcpclient.h"
#include "display.h"
#include "main.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "50100"
#define PORT_LOW 49152
#define PORT_HIGH 65535

STATIC int is_ipv4address(const char *address);
STATIC int is_port(const char *port);
STATIC int str_to_int(const char *string);

int create_connection(char *address, char *port)
{

    if (address == NULL) {
        address = DEFAULT_ADDRESS;
    }
    if (port == NULL) {
        port = DEFAULT_PORT;
    }
    if (!is_ipv4address(address)) {
        return -2;
    }
    if (!is_port(port)) {
        return -3;
    }

    // create client's TCP socket
    int clientSockFd = socket(AF_INET, SOCK_STREAM, 0); 
    if (clientSockFd < 0) {
        failed("Error creating socket.");
    }

    // initialize socket address structure with server's IP address and port
    struct sockaddr_in servaddr;

    memset((struct sockaddr_in *) &servaddr, 0, sizeof(struct sockaddr_in));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(str_to_int(port));

    int translationStatus = inet_pton(AF_INET, address, &servaddr.sin_addr);

    if (translationStatus <= 0) {
        failed("Translation failed for address: %s", address);
    }

    // establish connection with server
    int connectionStatus = connect(clientSockFd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr_in));

    return connectionStatus;
}

STATIC int is_ipv4address(const char *address) {
    
    int octets = 0;
    char *copy, *token, *savePtr;

    copy = strdup(address);

    token = strtok_r(copy, ".", &savePtr);

    while (token) {

        long n = str_to_int(token);

        if (n >= 0 && n <= 255) {
            octets++;
        }
        else {
            break;
        }
        token = strtok_r(NULL, ".", &savePtr);
    }

    free(copy);

    return octets == 4 && address[strlen(address) - 1] != '.';
}

STATIC int is_port(const char *port) {

    long n = str_to_int(port);

    if (n >= PORT_LOW && n <= PORT_HIGH) {
        return 1;
    }
    else {
        return 0;
    }  
}

STATIC int str_to_int(const char *string)
{

    char *end = NULL;
    errno = 0;

    long n = strtol(string, &end, 0);

    if (string == end || errno == ERANGE || *end != '\0' || n < 0) {
        return -1;
    }
    else {
        return n;
    }

}

