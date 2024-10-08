#include "tcpserver.h"
#include "user.h"
#include "channel.h"
#include "command_handler.h"

#include "../../shared/src/settings.h"
#include "../../shared/src/command.h"
#include "../../shared/src/message.h"
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
    TCPServer *tcpServer;
    ServerOptions options;
    CommandTokens *cmdTokens;
} AppState;

#ifndef TEST

static void daemonize(void);
static void cleanup(void);
static void get_options(int argc, char **argv, ServerOptions *options);

static AppState appState = {NULL, NULL, {.echo = 0, .daemon = 0} };

int main(int argc, char **argv)
{
    // register cleanup function
    atexit(cleanup);

    // set signal handler for SIGINT
    set_sigaction(handle_sigint, SIGINT);

    appState.logger = create_logger(NULL, LOG_FILE(server), DEBUG);

    // load settings
    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    // set default options
    appState.options.echo = 0;
    appState.options.daemon = 0;

    // get cli options
    get_options(argc, argv, &appState.options);

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
    appState.tcpServer = create_server(MAX_FDS);
    int listenFd = init_server();

    const int listenfdIndex = 0;

    // set listening socket fd
    set_fd(appState.tcpServer, listenfdIndex, listenFd);

    appState.cmdTokens = create_command_tokens();

    while (1) {

        // check for input events on file descriptors
        int fdsReady = poll(get_fds(appState.tcpServer), get_fds_capacity(appState.tcpServer), -1);
        if (fdsReady < 0) {
            FAILED("Error polling descriptors", NO_ERRCODE);  
        }
        /* remove clients which have not registered 
        connection in waiting time */
        const int waitingTime = 60;
        remove_inactive_clients(appState.tcpServer, waitingTime);

        Session *session = get_session(appState.tcpServer);
        
        int fdIndex = 0;

        while (fdsReady && fdIndex < get_fds_capacity(appState.tcpServer)) {

            int ready = is_fd_ready(appState.tcpServer, fdIndex);

            if (ready) {

                if (fdIndex == listenfdIndex) {
                    add_client(appState.tcpServer, listenfdIndex);
                }            
                else {

                    int fullMsg = server_read(appState.tcpServer, fdIndex);

                    /* if echo server is active and full message 
                    was received, send message back to the client */
                    if (fullMsg) {

                        Client *client = get_client(appState.tcpServer, fdIndex);
                        
                        if (appState.options.echo) {

                            server_write(get_client_inbuffer(client), get_client_fd(client));
                        }
                        else {

                            parse_message(appState.tcpServer, client, appState.cmdTokens);

                            CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(appState.cmdTokens));

                            LOG(INFO, "[%s] command parsed", command_type_to_string(commandType));

                            CommandFunction cmdFunction = get_command_function((CommandType)commandType);

                            cmdFunction(appState.tcpServer, client, appState.cmdTokens);

                            User *user = find_user_in_hash_table(session, get_client_nickname(client));

                            if (user != NULL && !is_queue_empty(get_user_queue(user))) {
                                add_user_to_ready_users(user, get_ready_list(session));
                            }

                            memset(get_client_inbuffer(client), '\0', MAX_CHARS + 1);

                            reset_cmd_tokens(appState.cmdTokens);
                        }
                    }
                }
                fdsReady--;
            }
            fdIndex++;
        }

        while (!is_queue_empty(get_msg_queue(appState.tcpServer))) {

            ExtMessage *message = remove_message_from_server_queue(appState.tcpServer);
            char *recipient = get_ext_message_recipient(message);
            char *content = get_ext_message_content(message);

            int fd = str_to_uint(recipient);

            server_write(content, fd);
        }

        iterate_list(get_ready_users(get_ready_list(session)), send_user_queue_messages, NULL);
        iterate_list(get_ready_channels(get_ready_list(session)), send_channel_queue_messages, get_session(appState.tcpServer));

        reset_linked_list(get_ready_users(get_ready_list(session)));
        reset_linked_list(get_ready_channels(get_ready_list(session)));

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

    delete_command_tokens(appState.cmdTokens);
    write_settings(NULL, SERVER_PROPERTY);
    delete_server(appState.tcpServer);
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