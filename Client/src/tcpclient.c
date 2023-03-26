#include "tcpclient.h"
#include "ui.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncursesw/curses.h>

void create_conection(WINDOW * topWin, WINDOW * bottomWin, const char *address, const char *port)
{

    int clientfd;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        failed("Error creating socket.");

    memset((struct sockaddr_in *) &serveraddr, 0, sizeof(struct sockaddr_in));
 
    int serverport = port == NULL ? SERVER_PORT : atoi(port);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(serverport);

    if (inet_pton(AF_INET, address, &serveraddr.sin_addr) <= 0)
        failed("Translation failed for address %s", address);

    display_infomsg(topWin, bottomWin, "Trying to connect...");

    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr_in)) == -1)
        display_infomsg(topWin, bottomWin, "Unable to connect to %s: %d", address, serverport);
    else
        display_infomsg(topWin, bottomWin, "Connected to %s: %d", address, serverport);

}

int is_valid_addr(const char *address) {
    
    int nbytes = 0;
    long n;
    char *copy, *token;

    copy = strdup(address);

    while ((token = strtok_r(copy, ".", &copy))) {
        if (is_number(token, &n) && n >= 0 && n <= 255)
            nbytes++;
    }

    return nbytes == 4 ? 1: 0;
}

int is_valid_port(const char *port) {

    long n;
    
    return (port == NULL || (is_number(port, &n) && n >= PORT_LOW && n <= PORT_HIGH)) ? 1 : 0;
}

int is_number(const char *string, long *n)
{
    char *nextchar;

    if (string != NULL)
        *n = strtol(string, &nextchar, 10);
    else
        return 0;

    return (nextchar == string || *nextchar != '\0' || ((*n == LONG_MIN || *n == LONG_MAX) && errno == ERANGE)) ? 0 : 1;

}

