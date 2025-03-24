#include "config.h"
#include "dispatcher.h"
#include "command_handler.h"

#include "../../libs/src/settings.h"
#include "../../libs/src/event.h"
#include "../../libs/src/command.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/time_utils.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>

/* references to dynamically allocated
    data for cleanup purposes */
typedef struct {
    struct itimerval *timer;
    EventManager *eventManager;
    Settings *settings;
    Logger *logger;
    StreamPipe *streamPipe;
    PollManager *pollManager;
    TCPClient *tcpClient;
    WindowManager *windowManager;
    CommandTokens *cmdTokens;
} AppContext;

#ifndef TEST

#define POLL_FD_COUNT 3

static AppContext appContext = {NULL};

static void process_input_data(int fdsReady);
static void cleanup(void);

int main(int argc, char **argv)
{
    /* register cleanup function */
    atexit(cleanup);
    
    /* craete interval timer for generating SIGALRM
        signal */
    appContext.timer = create_interval_timer(DEF_TIMER_INTERVAL);

    /* register event handlers */
    appContext.eventManager = create_event_manager(0);
    register_event_handlers(appContext.eventManager);

    /* initialize settings */
    appContext.settings = create_settings(CLIENT_OT_COUNT);
    initialize_client_settings();

    /*  get command line arguments */
    get_command_line_args(argc, argv);

    /* create logger and set logging options */
    LogLevel logLevel = get_int_option_value(OT_CLIENT_LOG_LEVEL);
    appContext.logger = create_logger(NULL, LOG_FILE(client), logLevel);
    enable_stdout_logging(0);

    LOG(INFO, "Client started");

    /*  a pipe is used to handle registered signals 
        with poll(). signals interrupt poll() and
        transfer control to the signal handler. inside
        a registered handler, a message is written to 
        the pipe. this message will then be detected 
        with poll() as an input event on the pipe */
    appContext.streamPipe = create_pipe();
    set_client_pipe_fd(get_pipe_fd(appContext.streamPipe, WRITE_PIPE));

    /* poll manager tracks input activity on stdin, socket and 
        pipe fd's */
    appContext.pollManager = create_poll_manager(POLL_FD_COUNT, POLLIN);

    /* set fd for stdin and pipe */
    set_poll_fd(appContext.pollManager, STDIN_FILENO);
    set_poll_fd(appContext.pollManager, get_pipe_fd(appContext.streamPipe, READ_PIPE));

    /* tcp client provides networking functionality 
        for the app */
    appContext.tcpClient = create_client();

    /* set signal handlers */
    set_sigaction(handle_client_sigint, SIGINT, (int[]){SIGINT, SIGTERM, SIGHUP, SIGQUIT, 0});
    set_sigaction(handle_sigalrm, SIGALRM, (int[]){SIGALRM, 0});
    set_sigaction(handle_sigwinch, SIGWINCH, (int[]){SIGWINCH, 0});
    set_sigaction(SIG_IGN, SIGPIPE, NULL);
  
    /* create UI */
    int sbMultiplier = get_int_option_value(OT_SCROLLBACK);
    int cmdHistoryCount = get_int_option_value(OT_HISTORY);

    appContext.windowManager = create_window_manager(sbMultiplier, cmdHistoryCount);
    set_windows_options(appContext.windowManager);
    init_colors(get_int_option_value(OT_COLOR));
    init_ui(appContext.windowManager, get_int_option_value(OT_COLOR));

    /* command tokens store parsed commands */
    appContext.cmdTokens = create_command_tokens(1);

    set_event_context(appContext.eventManager, appContext.windowManager, appContext.pollManager, appContext.tcpClient, appContext.cmdTokens);

    /* client uses event driven programming to handle I/O events.
        the program workflow consists of the following actions:

        1.  wait for input events on stdin, socket and pipe fd's,
        2.  read data from stdin, socket or pipe fd's,
        3.  create input events and add them to event queue,
        4.  process events from the event queue and dispatch them to event handlers,
        5.  send IRC messages to the server */

    while (1) {

        /*  client uses I/O multiplexing with poll() to monitor stdin,
            pipe and socket fd's for input events (readiness to read data) */
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

        process_input_data(fdsReady);

        dispatch_events(appContext.eventManager);
        send_socket_messages(appContext.eventManager, appContext.tcpClient);
    }
	return 0;
}

static void process_input_data(int fdsReady) {

    if (fdsReady && is_fd_input_event(appContext.pollManager, STDIN_FILENO)) {
        process_stdin_data(appContext.eventManager, appContext.windowManager, appContext.tcpClient, appContext.cmdTokens);
        fdsReady--;
    }
    if (fdsReady && is_fd_input_event(appContext.pollManager, get_pipe_fd(appContext.streamPipe, READ_PIPE))) {
        process_pipe_data(appContext.eventManager, appContext.streamPipe);
        fdsReady--;
    }
    if (fdsReady && is_fd_input_event(appContext.pollManager, get_client_fd(appContext.tcpClient))) {
        process_socket_data(appContext.eventManager, appContext.windowManager, appContext.tcpClient);
        fdsReady--;
    }
}

/*  save settings and perform cleanup */
static void cleanup(void) {

    LOG(INFO, "Terminated");

    if (appContext.settings != NULL) {
        write_settings(NULL);
    }

    delete_command_tokens(appContext.cmdTokens);
    delete_window_manager(appContext.windowManager);
    delete_client(appContext.tcpClient);
    delete_poll_manager(appContext.pollManager);
    delete_pipe(appContext.streamPipe);
    delete_logger(appContext.logger);
    delete_settings(appContext.settings);
    delete_event_manager(appContext.eventManager);
    delete_interval_timer(appContext.timer);
}

#endif
