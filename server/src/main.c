// #define _XOPEN_SOURCE 700

#include "tcpserver.h"
#include "../../shared/src/signal_handler.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <signal.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef TEST
    #define LOG_FILE(str) "test_" #str
#else
    #define LOG_FILE(str) #str
#endif

typedef struct {
    Logger *logger;
    PollFds *pollFds;
} AppState;

#ifndef TEST

static void daemonize(void);
// static void set_sigaction(void (*handler)(int), int sig);
// static void handle_sigint(int sig);

static AppState appState = {NULL};
static void cleanup(void);

int main(int argc, char **argv)
{

    // register cleanup function
    atexit(cleanup);

    // set signal handler for SIGINT
    set_sigaction(handle_sigint, SIGINT);

    int opt, daemonServer = 0, echoServer = 0;

    // get cmd options
    while ((opt = getopt(argc, argv, "de")) != -1) {

        switch (opt) {
            case 'd': {
                daemonServer = 1;
                break;
            }
            case 'e': {
                echoServer = 1;
                break;
            }
            default:
                printf("Usage: %s [-d] [-e]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    // create daemon process
    if (daemonServer) {
        daemonize();
        set_stderr_allowed(0);
    }
    
    appState.logger = create_logger(NULL, LOG_FILE("server"));

    LOG(INFO, "Server options: daemon %d, echo %d", daemonServer, echoServer);

    appState.pollFds = create_pfds(0);

    int listenFd = init_server();

    // set listening socket fd
    set_pfd(appState.pollFds, 0, listenFd, POLLIN);

    while (1) {

        // poll file descriptors for events
        int fdsReady = poll(get_pfds(appState.pollFds), get_allocated_pfds(appState.pollFds), -1);
        if (fdsReady < 0) {
            FAILED("Error polling descriptors", NO_ERRCODE);  
        }

        check_listening_pfd(appState.pollFds, &fdsReady);
        check_connected_pfds(appState.pollFds, &fdsReady, echoServer);
    }

    return 0;
}

static void cleanup(void) {

    delete_pfds(appState.pollFds);
    delete_logger(appState.logger);
}

static void daemonize(void) {

    pid_t pid = fork();

    if (pid < 0) {
        FAILED("Error creating daemon process", NO_ERRCODE);
    }
    else if (pid) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        FAILED("Error creating daemon process", NO_ERRCODE);
    }

    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0) {
        FAILED("Error creating daemon process", NO_ERRCODE);
    }
    else if (pid) {
        exit(EXIT_SUCCESS);
    }

    const int MAX_FD = 64;
    for (int i = 0; i < MAX_FD; i++) {
        close(i);
    }

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

}

// static void set_sigaction(void (*handler)(int), int sig) {

//     struct sigaction sa;

//     sa.sa_handler = handler;
//     sa.sa_flags = 0;
//     sigemptyset(&sa.sa_mask);

//     if (sigaction(sig, &sa, NULL) == -1) {
//         FAILED("Failed to set signal handler", NO_ERRCODE);
//     }
// }

// /* call
// invoke cleanup function on exit */
// static void handle_sigint(int sig) {

//     exit(EXIT_SUCCESS);
// }

#endif


