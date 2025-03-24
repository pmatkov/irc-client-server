#include "config.h"
#include "tcp_server.h"
#include "dispatcher.h"
#include "../../libs/src/event.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/command.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>

typedef struct {
    EventManager *eventManager;
    Settings *settings;
    Logger *logger;
    StreamPipe *streamPipe;
    TCPServer *tcpServer;
    PollManager *pollManager;
    CommandTokens *cmdTokens;
} AppContext;

#ifndef TEST

static AppContext appContext = {NULL};

static void run_standard_server(void);
static void cleanup(void);

int main(int argc, char **argv)
{
    /* register cleanup function */
    atexit(cleanup);

    /* register event handlers */
    appContext.eventManager = create_event_manager(0);
    register_event_handlers(appContext.eventManager);

    /* initialize settings */
    appContext.settings = create_settings(SERVER_OT_COUNT);
    initialize_server_settings();

    /* parse command line arguments */
    get_command_line_args(argc, argv);

    /* create logger and set logging options */
    LogLevel logLevel = get_int_option_value(OT_SERVER_LOG_LEVEL);
    appContext.logger = create_logger(NULL, LOG_FILE(server), logLevel);

    LOG(INFO, "Server started");

    if (get_int_option_value(OT_DAEMON)) {
        daemonize();
        LOG(INFO, "Daemon process started (pid: %d)", getpid());
    }
    if (get_int_option_value(OT_ECHO)) {
        LOG(INFO, "Echo server enabled");
    }
    if (get_int_option_value(OT_THREADS)) {
        LOG(INFO, "Multithreading enabled");
    }

    /*  a pipe is used to handle registered signals 
        with poll(). signals interrupt poll() and
        transfer control to the signal handler. inside
        a registered handler, a message is written to 
        the pipe. this message will then be detected 
        with poll() as an input event on the pipe */
    appContext.streamPipe = create_pipe();
    set_server_pipe_fd(get_pipe_fd(appContext.streamPipe, WRITE_PIPE));

    /* poll manager tracks input activity on socket and 
        pipe fd's */
    appContext.pollManager = create_poll_manager(get_int_option_value(OT_MAX_FDS), POLLIN);

    /* tcpServer provides networking functionality for 
        the app */
    appContext.tcpServer = create_server(get_int_option_value(OT_MAX_FDS));
    int listenFd = init_server(appContext.tcpServer, NULL, get_int_option_value(OT_PORT));

    /* set fd for pipe and listening socket */
    set_poll_fd(appContext.pollManager, get_pipe_fd(appContext.streamPipe, READ_PIPE));
    set_poll_fd(appContext.pollManager, listenFd);

    /* set signal handlers */
    set_sigaction(handle_server_sigint, SIGINT, (int[]){SIGINT, SIGTERM, SIGHUP, SIGQUIT, 0});
    set_sigaction(handle_server_sigint, SIGTERM, (int[]){SIGINT, SIGTERM, SIGHUP, SIGQUIT, 0});
    set_sigaction(SIG_IGN, SIGPIPE, NULL);

    /* command tokens store parsed commands */
    appContext.cmdTokens = create_command_tokens(1);

    set_event_context(appContext.eventManager, appContext.pollManager, appContext.tcpServer, appContext.cmdTokens);

    if (!get_int_option_value(OT_THREADS)) {
        run_standard_server();  
    }
    return 0;
}

/* a standard server runs on a single thread */
static void run_standard_server(void) {

  /* server uses event driven programming to handle I/O events.
    the program workflow consists of the following actions:

        1. wait for input events on pipe and socket fd's,
        2. read data from pipe and socket fd,
        3. create input events and add them to event queue,
        4.  process events from the event queue and dispatch them to event handlers,
        5.  send IRC responses to clients */

    while (1) {

        /*  server uses I/O multiplexing with poll() to monitor pipe and
            socket fd's for input events (readiness to read data). 
            pipe is used for handling signals with poll(). sockets are
            used for accepting connection requests from clients and 
            exchanging data with clients */
        int fdsReady = poll(get_poll_pfds(appContext.pollManager), get_poll_fd_count(appContext.pollManager), -1);

        if (fdsReady < 0) {

            /* restart poll() if it was interrupted by a signal */
            if (errno == EINTR) {
                continue;
            }
            else {
                FAILED(NO_ERRCODE, "Error polling descriptors");  
            }
        }

        if (fdsReady && is_fd_input_event(appContext.pollManager, get_pipe_fd(appContext.streamPipe, READ_PIPE))) {
            process_pipe_data(appContext.eventManager, appContext.streamPipe);
            fdsReady--;
        }

        if (fdsReady && is_fd_input_event(appContext.pollManager, get_server_listen_fd(appContext.tcpServer))) {
            process_connection_request(appContext.eventManager, appContext.tcpServer);
            fdsReady--;
        }

        int connectedFd = get_server_listen_fd(appContext.tcpServer) + 1;

        while (fdsReady && connectedFd < get_server_capacity(appContext.tcpServer)) {

            if (is_fd_input_event(appContext.pollManager, connectedFd)) {

                process_socket_data(appContext.eventManager, appContext.tcpServer, connectedFd);
                fdsReady--;
            }
            else if (is_fd_error_event(appContext.pollManager, connectedFd)) {

                trigger_event_client_disconnect(appContext.eventManager, connectedFd);
                fdsReady--;
            }
            connectedFd++;
        }

        dispatch_events(appContext.eventManager);
        send_socket_messages(appContext.eventManager, appContext.tcpServer);
    }
}

/* perform cleanup */
static void cleanup(void) {

    if (!get_int_option_value(OT_DAEMON)) {
        LOG(INFO, "Terminated");
    }

    delete_command_tokens(appContext.cmdTokens);
    delete_poll_manager(appContext.pollManager);
    delete_server(appContext.tcpServer);
    delete_pipe(appContext.streamPipe);
    delete_logger(appContext.logger);
    delete_settings(appContext.settings);
    delete_event_manager(appContext.eventManager);
}

#endif