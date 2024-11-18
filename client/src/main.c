#include "main.h"
#include "tcp_client.h"
#include "display.h"
#include "scrollback.h"
#include "line_editor.h"
#include "command_handler.h"
#include "tcp_client.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
#include "../../libs/src/time_utils.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <pwd.h>
#include <assert.h>

/* contains command line arguments */
// typedef struct {
//     int color;
//     int sbMultiplier;
//     LogLevel logLevel;
// } CLArgs;

/* holds references to dynamically allocated
    data for cleanup */
typedef struct {
    struct itimerval *intervalTimer;
    Logger *logger;
    Settings *settings;
    TCPClient *tcpClient;
    WindowManager *windowManager;
    CommandTokens *cmdTokens;
} AppContext;

#ifndef TEST

static AppContext appContext = {NULL};

/* enables signal handling with poll() - 
   self pipe trick */
static int clientPipeFd[PIPE_FD_COUNT];

static void read_keyboard_input(TCPClient *tcpClient, WindowManager *windowManager, CommandTokens *cmdTokens);
static void read_socket_messages(TCPClient *tcpClient, WindowManager *windowManager);
// static void read_pipe_messages(WindowManager *windowManager, CLArgs *clArgs);
static void read_pipe_messages(WindowManager *windowManager);
static void send_socket_messages(TCPClient *tcpClient);
static void handle_signal(const char *signalId, void *arg);

// static void get_command_line_args(int argc, char **argv, CLArgs *clArgs);
static void get_command_line_args(int argc, char **argv);
static void cleanup(void);

int main(int argc, char **argv)
{
    /* register cleanup function */
    atexit(cleanup);

    appContext.intervalTimer = create_interval_timer(DEF_INTERVAL);

    // CLArgs clArgs = {.color = 1, .sbMultiplier = 5, .logLevel = DEBUG};
    
    /* create and initialize settings */
    appContext.settings = create_settings(CLIENT_OT_COUNT);
    initialize_client_settings();

    /* parse command line arguments */
    get_command_line_args(argc, argv);

    /* create logger and set logging options */
    appContext.logger = create_logger(NULL, LOG_FILE(client),  get_int_option_value(OT_CLIENT_LOG_LEVEL));
    enable_stdout_logging(0);

    LOG(INFO, "Client started");

    /* provides networking functionality 
        for the app */
    appContext.tcpClient = create_client();

    /*  a pipe is used to handle registered signals 
        with poll(). signals interrupt poll() and
        transfer control to the signal handler. inside
        a registered handler, a message is written to 
        the pipe. this message will then be detected 
        with poll() as an input event on the pipe */
    create_pipe(clientPipeFd);

    /* set fd for read end of the pipe */
    set_fd(appContext.tcpClient, PIPE_FD_IDX, clientPipeFd[READ_PIPE]);

    set_client_pipe(clientPipeFd[WRITE_PIPE]);

    /* set signal handlers */
    set_sigaction(handle_client_sigint, SIGINT, (int[]){SIGINT, SIGTERM, SIGHUP, SIGQUIT, 0});
    set_sigaction(handle_sigalrm, SIGALRM, (int[]){SIGALRM, 0});
    set_sigaction(handle_sigwinch, SIGWINCH, (int[]){SIGWINCH, 0});
    set_sigaction(SIG_IGN, SIGPIPE, NULL);
  
    /* create windows, set window options and
        initialize colors */
    appContext.windowManager = create_windows(get_int_option_value(OT_MULTIPLIER));
    set_windows_options(appContext.windowManager);
    init_colors(get_int_option_value(OT_COLOR));

    /* initialize user interface */
    init_ui(appContext.windowManager, get_int_option_value(OT_COLOR));

    /* command tokens store parsed commands */
    appContext.cmdTokens = create_command_tokens(1);

    /* the main program flow consists of the following actions:

        1. read and parse keyboard input,
        2.a if the input is a local command, execute it,
        2.b if the input is an IRC command, transform it, if required,
            and add it to the message queue,
        3.a establish a connection to the server, if not already 
            established,
        3.b send commands to the server,
        4. read incoming server messages and execute them */

    while (1) {

        /* poll() is used to monitor input events on the pipe, the 
            stdin and the network socket */
        int fdsReady = poll(get_fds(appContext.tcpClient), POLL_FD_COUNT, -1);

        if (fdsReady < 0) {

            /* restart poll() if interrupted by a signal */
            if (errno == EINTR) {
                continue;

                // if (get_resized()) {

                //     /* reinitialize UI if poll() was interrupted by the 
                //         SIGWINCH signal (i.e., the window was resized) */
                //     resize_ui(appContext.windowManager, clArgs.color);
                //     set_resized(0);
                // }
            }
            else {
                FAILED(NO_ERRCODE, "Error polling descriptors");  
            }
        }
        if (is_stdin_event(appContext.tcpClient)) {
            read_keyboard_input(appContext.tcpClient, appContext.windowManager, appContext.cmdTokens);
        }

        if (is_socket_event(appContext.tcpClient)) {
            read_socket_messages(appContext.tcpClient, appContext.windowManager);
        }

        if (is_pipe_event(appContext.tcpClient)) {
            read_pipe_messages(appContext.windowManager);
        }

        if (!is_queue_empty(get_client_queue(appContext.tcpClient))) {
            send_socket_messages(appContext.tcpClient);
        }
    }
	return 0;
}

/* read keystrokes */
static void read_keyboard_input(TCPClient *tcpClient, WindowManager *windowManager, CommandTokens *cmdTokens) {

    if (tcpClient == NULL || windowManager == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* read char from the keyboard and execute the
        appropriate action. if a control char was read 
        (e.g. arrow, backspace etc), a control action
        will be executed, if a regular char was read, 
        it will be added to the buffer, if a newline 
        char was read, the whole input line will be 
        parsed */
    int ch, index;

    ch = wgetch(get_window(get_inputwin(windowManager)));
    ch = remap_ctrl_key(ch);

    if ((index = get_sb_func_index(ch)) != -1) {
        get_scrollback_function(index)(get_scrollback(get_chatwin(windowManager)));
    }
    else if ((index = get_le_func_index(ch)) != -1) {
        get_lneditor_function(index)(get_line_editor(get_inputwin(windowManager)));
    }    
    else if (isprint(ch)) {
        add_char(get_line_editor(get_inputwin(windowManager)), ch);
    }
    else if (ch == KEY_NEWLINE) {
        parse_input(get_line_editor(get_inputwin(windowManager)), cmdTokens);
    } 

    const char *commandString = get_command(cmdTokens);

    /* parse and execute commands */
    if (commandString != NULL) {

        CommandFunc commandFunc = get_command_function(string_to_command_type(commandString));
        commandFunc(windowManager, tcpClient, cmdTokens);

        reset_command_tokens(cmdTokens);
    }

    wrefresh(get_window(get_inputwin(windowManager)));
}

/* read messages from the socket */
static void read_socket_messages(TCPClient *tcpClient, WindowManager *windowManager) {

    if (tcpClient == NULL || windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* read socket data */
    int readStatus = client_read(tcpClient);

    if (readStatus == -1) {
        display_response(windowManager, "Server terminated.");
        display_status(windowManager, "");
    }
    else if (readStatus == 1) {

        /* extract messages from incoming data.
            partial messages are left in the buffer */
        char *buffer = get_client_inbuffer(tcpClient);
        process_messages(buffer, CRLF, display_server_message, windowManager);
    }
}

/* read messages from the pipe */
static void read_pipe_messages(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char buffer[MAX_CHARS + 1] = {'\0'};

    read_string(clientPipeFd[READ_PIPE], buffer, sizeof(buffer) - 1);
    
    struct {
        WindowManager *windowManager;
        TCPClient *tcpClient;
    } data = {windowManager};

    process_messages(buffer, CRLF, handle_signal, &data);
}

/* send messages to the socket */
static void send_socket_messages(TCPClient *tcpClient) {

    if (tcpClient == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    while (!is_queue_empty(get_client_queue(tcpClient))) {

        char *message = remove_message_from_client_queue(tcpClient);
        client_write(tcpClient, get_socket_fd(tcpClient), message);
    } 
}

static void handle_signal(const char *label, void *arg) {

    if (label == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        WindowManager *windowManager;
        TCPClient *tcpClient;
    } *data = arg;

    if (strcmp(label, "sigint") == 0) {
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(label, "sigalrm") == 0) {
        display_time(data->windowManager);
    } 
    else if (strcmp(label, "sigwinch") == 0) {
        resize_ui(data->windowManager, get_int_option_value(OT_COLOR));
    } 
}

static void get_command_line_args(int argc, char **argv) {

    int opt;

    static struct option longOptions[] = {
        {"nocolor", no_argument, NULL, 'c'},
        {"loglevel", required_argument, NULL, 'l'},
        {"scrollback", required_argument, NULL, 's'},
        {0, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "", longOptions, NULL)) != -1) {
        
        switch (opt) {    
            case 'c':
                if (optarg) {
                    int color = str_to_uint(optarg);
                    if (color == 0 || color == 1) {
                        set_option_value(OT_COLOR, &color);
                    }
                }
                // clArgs->color = 0;
                break;
            case 'l':
                if (optarg) {
                    LogLevel loglevel = string_to_log_level(optarg);
                    if (is_valid_log_level(loglevel)) {
                        set_option_value(OT_CLIENT_LOG_LEVEL, &loglevel);
                    }
                }
                // clArgs->logLevel = string_to_log_level(optarg);

                break;
            case 's':
                if (optarg) {
                    int multiplier = str_to_uint(optarg);
                    if (multiplier >= 1 && multiplier <= 5) {
                        set_option_value(OT_MULTIPLIER, &multiplier);
                    }
                }
                // if (optarg && (str_to_uint(optarg) >= 1 || str_to_uint(optarg) <= 5)) {
                //     clArgs->sbMultiplier = str_to_uint(optarg);
                // }
                break;
            default:
                printf("Usage: %s [--nocolor] [--scrollback <1-5>] [--loglevel <debug | info | warning | error>]\n", argv[0]);
                printf("\tOptions:\n");
                printf("\t  --nocolor       : Disable colors for the user interface\n");
                printf("\t  --scrollback:   : Set the scrollback buffer multiplier\n");
                printf("\t  --loglevel      : Set the logging level\n");
                exit(EXIT_FAILURE);
        }
    }
}

/*  save settings and perform cleanup */
static void cleanup(void) {

    LOG(INFO, "Shutdown initiated");

    close(clientPipeFd[READ_PIPE]);
    close(clientPipeFd[WRITE_PIPE]);

    close(get_stdin_fd(appContext.tcpClient));

    if (appContext.settings != NULL) {
        write_settings(NULL);
    }

    delete_command_tokens(appContext.cmdTokens);
    delete_windows(appContext.windowManager);
    delete_client(appContext.tcpClient);
    delete_logger(appContext.logger);
    delete_settings(appContext.settings);
    delete_interval_timer(appContext.intervalTimer);
}

#endif

void initialize_client_settings(void) {

    /* get username of the current user */
    struct passwd *userRecord = getpwuid(getuid());
    char *name = userRecord != NULL ? userRecord->pw_name : "";

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", name);
    register_option(CHAR_TYPE, OT_USERNAME, "username", name);
    register_option(CHAR_TYPE, OT_REALNAME, "realname", "anonymous");
    register_option(CHAR_TYPE, OT_SERVER_ADDRESS, "address", "localhost");
    register_option(INT_TYPE, OT_SERVER_PORT, "port", &(int){50100});
    register_option(INT_TYPE, OT_COLOR, "color", &(int){1});
    register_option(INT_TYPE, OT_CLIENT_LOG_LEVEL, "loglevel", &(int){DEBUG});
    register_option(INT_TYPE, OT_MULTIPLIER, "multiplier", &(int){5});

    read_settings(NULL);
}

// int get_client_pipe_fd(void) {

//     return clientPipeFd[WRITE_PIPE];
// }
