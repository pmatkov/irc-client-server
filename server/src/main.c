#include "tcpserver.h"
#include "user.h"
#include "channel.h"

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

typedef struct {
    int echo;
    int daemon;
} ServerOptions;

typedef struct {
    Logger *logger;
    PollFdSet *pollFdSet;
    TCPServer *tcpServer;
    ServerOptions options;
} AppState;

#ifndef TEST

static void daemonize(void);
static void cleanup(void);
static void get_options(int argc, char **argv, ServerOptions *options);

static AppState appState = {NULL, NULL, NULL, {.echo = 0, .daemon = 0} };

int main(int argc, char **argv)
{
    // register cleanup function
    atexit(cleanup);

    // set signal handler for SIGINT
    set_sigaction(handle_sigint, SIGINT);

    // get server options
    get_options(argc, argv, &appState.options);

    appState.logger = create_logger(NULL, LOG_FILE(server), DEBUG);

    // create daemon process
    if (appState.options.daemon) {
        daemonize();
        set_sigaction(handle_sigint, SIGTERM);
        LOG(INFO, "Started daemon process with PID: %d", getpid());
    }

    if (appState.options.echo) {
        LOG(INFO, "Echo server enabled");
    }

    /* create fd's set to monitor socket events -
     connection requests and messages from connected
      clients */
    appState.pollFdSet = create_pollfd_set(MAX_FDS);
    appState.tcpServer = create_server("irc.example.com", MAX_FDS);
    int listenFd = init_server();

    // add_channel(appState.channelList, "#general", PERMANENT);

    // set listening socket fd
    set_pfd(appState.pollFdSet, 0, listenFd, POLLIN);

    while (1) {

        // poll fd's for events
        int fdsReady = poll(get_pfds(appState.pollFdSet), get_pfds_capacity(appState.pollFdSet), -1);
        if (fdsReady < 0) {
            FAILED("Error polling descriptors", NO_ERRCODE);  
        }

        handle_inactive_clients(appState.pollFdSet, appState.tcpServer);
        check_listening_pfd(appState.pollFdSet, appState.tcpServer, &fdsReady);
        check_connected_pfds(appState.pollFdSet, appState.tcpServer, &fdsReady, appState.options.echo);
    }

    return 0;
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

    set_stderr_allowed(0);
    set_stdout_allowed(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

}

static void cleanup(void) {

    delete_server(appState.tcpServer);
    delete_pollfd_set(appState.pollFdSet);
    delete_logger(appState.logger);
}

static void get_options(int argc, char **argv, ServerOptions *options) {

    int opt;

    while ((opt = getopt(argc, argv, "de")) != -1) {

        switch (opt) {
            case 'd': {
                options->daemon = 1;
                break;
            }
            case 'e': {
                options->echo = 1;
                break;
            }
            default:
                printf("Usage: %s [-d] [-e]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

#endif