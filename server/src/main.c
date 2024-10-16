#include "main.h"
#include "tcp_server.h"
#include "user.h"
#include "channel.h"
#include "command_handler.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

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

/* serverOptions contains command line
    options */
typedef struct {
    int echo;
    int daemon;
    int port;
    int maxFds;
    const char *hostname;
    int waitTime;
} ServerOptions;

/* appStat holds references to heap
    allocated data for cleanup purposes */
typedef struct {
    Logger *logger;
    TCPServer *tcpServer;
    CommandTokens *cmdTokens;
} AppState;

#ifndef TEST

static void daemonize(void);
static void get_options(int argc, char **argv, ServerOptions *options);
static void cleanup(void);

static AppState appState = {NULL, NULL, NULL};

int main(int argc, char **argv)
{
    /* register the cleanup function */
    atexit(cleanup);

    /* set signal handler for SIGINT (program 
        interrupt) signal */
    set_sigaction(handle_sigint, SIGINT);

    /* set default values for the server's options */
    ServerOptions options = 
        {.echo = 0, .daemon = 0, .port = 50100, 
        .maxFds = 1024, .hostname = "irc.server.com", 
        .waitTime = 60};

    /* parse command line arguments */
    get_options(argc, argv, &options);

    appState.logger = create_logger(NULL, LOG_FILE(server), DEBUG);

    /* if the required command line arguments 
        are present, create a daemon process
        and/ or enable the echo server */
    if (options.daemon) {
        daemonize();
        set_sigaction(handle_sigint, SIGTERM);
        LOG(INFO, "Started daemon process with PID: %d", getpid());
    }
    if (options.echo) {
        LOG(INFO, "Echo server enabled");
    }

    /* tcp server provides networking 
        functionality for the app */
    appState.tcpServer = create_server(options.maxFds, options.hostname);
    int listenFd = init_server(options.port);

    const int listenfdIndex = 0;

    /* set fd for the listening socket */
    set_fd(appState.tcpServer, listenfdIndex, listenFd);

    /* command tokens contain parsed command
        tokens */
    appState.cmdTokens = create_command_tokens();

    /* the basic program flow includes the 
        following actions:

        1.a accept client connection requests,
        1.b read client messages,
        2.  parse messages,
        3. if a message is a valid IRC command, execute it,
        4. send response(s) to the client that issued the
            command or to other client(s) */

    while (1) {

        /* poll() is used to monitor input events on socket 
            file descriptors. Two types of sockets are monitored: 
            the 'listening' socket, that receives connection 
            requests from the clients and the 'connected' 
            socket, that transmits data to and from the client. 
            
            the effectiveness of poll() in monitoring file 
            descriptors starts to degrade after about 1000 fd's.
            therefore, a maximum value of fd's is set at 1024
            (1023 clients + 1 for the listening socket) */
        int fdsReady = poll(get_fds(appState.tcpServer), get_fds_capacity(appState.tcpServer), -1);
        if (fdsReady < 0) {
            FAILED("Error polling descriptors", NO_ERRCODE);  
        }

        /* remove clients that have not registered their connection
            in due time */
        remove_inactive_clients(appState.tcpServer, options.waitTime);

        Session *session = get_session(appState.tcpServer);
        
        int fdIndex = 0;

        while (fdsReady && fdIndex < get_fds_capacity(appState.tcpServer)) {

            int ready = is_fd_ready(appState.tcpServer, fdIndex);

            if (ready) {

                /* if input activity is detected at the 'listening' 
                    socket, it should be interpreted as a connection 
                    request. therefore, a new client will be added.
                    otherwise, the actity indicates incoming data at the
                    'connected socket'. in that case, this data should
                    be read */
                if (fdIndex == listenfdIndex) {
                    add_client(appState.tcpServer, listenfdIndex);
                }            
                else {

                    int fullMsg = server_read(appState.tcpServer, fdIndex);

                    /* if the echo server is enabled and the full message 
                    was received, echo message back to the client. otherwise,
                    parse the message expecting the client's command and proceed 
                    accordingly */
                    if (fullMsg) {

                        Client *client = get_client(appState.tcpServer, fdIndex);
                        
                        if (options.echo) {

                            server_write(get_client_inbuffer(client), get_client_fd(client));
                        }
                        else {

                            parse_message(appState.tcpServer, client, appState.cmdTokens);

                            /* verify if a valid command was parsed,
                                and if so, execute it */
                            CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(appState.cmdTokens));

                            LOG(INFO, "[%s] command parsed", command_type_to_string(commandType));

                            CommandFunc cmdFunction = get_command_function((CommandType)commandType);

                            cmdFunction(appState.tcpServer, client, appState.cmdTokens);

                            /* if the user has pending messages, add him to the list
                                of users with messages ready for sending */
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

        /* send messages from the server queue (these are the
            messages intended for clients which have not yet 
            registered their connection) */
        while (!is_queue_empty(get_msg_queue(appState.tcpServer))) {

            ExtMessage *message = remove_message_from_server_queue(appState.tcpServer);
            char *recipient = get_ext_message_recipient(message);
            char *content = get_ext_message_content(message);

            int fd = str_to_uint(recipient);

            server_write(content, fd);
        }

        /* send messages from the users' and channels' queue (each 
            user and channel has a dedicated message queue) */
        iterate_list(get_ready_users(get_ready_list(session)), send_user_queue_messages, NULL);
        iterate_list(get_ready_channels(get_ready_list(session)), send_channel_queue_messages, get_session(appState.tcpServer));

        reset_linked_list(get_ready_users(get_ready_list(session)));
        reset_linked_list(get_ready_channels(get_ready_list(session)));

    }

    return 0;
}


/* create a deamon process that will run in the background
    detached from the terminal */
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
    set_use_stdout(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

}

/* parse command line parameters.
    hos*/
static void get_options(int argc, char **argv, ServerOptions *options) {

    int opt;

    while ((opt = getopt(argc, argv, "defhpw")) != -1) {

        switch (opt) {
            case 'd': {
                options->daemon = 1;
                break;
            }
            case 'e': {
                options->echo = 1;
                break;
            }
            case 'f': {
                int maxFds = str_to_uint(argv[3]);
                if (maxFds != -1) {
                    options->maxFds = maxFds;
                }
                break;
            }
            case 'h': {
                options->hostname = argv[4];
                break;
            }
            case 'p': {
                int port = str_to_uint(argv[5]);
                if (port != -1 && is_valid_port(port)) {
                    options->port = port;
                }
                break;
            }
            case 'w': {
                int waitTime = str_to_uint(argv[6]);
                if (waitTime != -1) {
                    options->waitTime = waitTime;
                }
                break;
            }
            default:
                printf("Usage: %s [-d <daemon>] [-e <echo>] [-f <max_fds>] [-h <hostname>] [-p <port>] [-w <waittime>]\n", argv[0]);
                printf("\tOptions:\n");
                printf("\t  -d : Run as a daemon\n");
                printf("\t  -e : Enable echo mode\n");
                printf("\t  -f : Set maximum file descriptors\n");
                printf("\t  -h : Specify the hostname\n");
                printf("\t  -p : Specify the port number\n");
                printf("\t  -w : Set wait time\n");
                exit(EXIT_FAILURE);
        }
    }
}

/* release heap allocated memory */
static void cleanup(void) {

    delete_command_tokens(appState.cmdTokens);
    delete_server(appState.tcpServer);
    delete_logger(appState.logger);
}

#endif
