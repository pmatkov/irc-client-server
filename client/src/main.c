// #include "settings.h"
#include "tcpclient.h"
#include "display.h"
#include "scrollback.h"
#include "line_editor.h"
#include "command_handler.h"
#include "../../shared/src/settings.h"
#include "../../shared/src/command.h"
#include "../../shared/src/message.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/signal_handler.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#define PFDS_COUNT 2

/* stores command line arguments */
typedef struct {
    int color;
    int scrollback;
    LogLevel logLevel;
} ClientOptions;

/* defines references to heap allocated 
 data for cleanup purpose */
typedef struct {
    Logger *logger;
    TCPClient *tcpClient;
    WindowManager *windowManager;
    Scrollback *scrollback;
    LineEditor *lnEditor;
    ClientOptions options;
    CommandTokens *cmdTokens;
} AppState;

#ifndef TEST

static AppState appState = {NULL};

static void cleanup(void);
static void get_options(int argc, char **argv, ClientOptions *options);

int main(int argc, char **argv)
{
    /* registers cleanup function */
    atexit(cleanup);

    /* sets signal handlers for SIGINT (keyboard 
        interrupt) and SIGWINCH (resize
        event) signals */
    set_sigaction(handle_sigint, SIGINT);
    set_sigaction(handle_sigwinch, SIGWINCH);

    /* set default command line arguments */
    appState.options.color = 1;
    appState.options.scrollback = 5;
    appState.options.logLevel = DEBUG;

    /* gets command line arguments */
    get_options(argc, argv, &appState.options);

    /* creates logger, set log file appendix and logging
        level and disable stdout logging */
    appState.logger = create_logger(NULL, LOG_FILE(client), appState.options.logLevel);
    set_stdout_allowed(0);

    LOG(INFO, "Client started");

    /* sets default client settings and loads
        settings from file if available */
    set_default_settings();
    read_settings(NULL, CLIENT_PROPERTY);

    /* tcp client provides networking 
        functionality */
    appState.tcpClient = create_client();

    /* creates UI with ncurses library */
    appState.windowManager = create_windows();
    set_windows_options(appState.windowManager);
    init_colors(appState.options.color);

    appState.scrollback = create_scrollback(get_chatwin(appState.windowManager), appState.options.scrollback);
    appState.lnEditor = create_line_editor(get_inputwin(appState.windowManager));
    create_layout(appState.windowManager, appState.scrollback, appState.options.color);

    /* command tokens are used to parse 
        user commands */
    appState.cmdTokens = create_command_tokens();

    /* the main program flow consists of the following:
        1. read keyboard input,
        2. parse input,
        3. if input is an IRC command, format it 
            and add it to the message queue,
        4. send messages from the queue to the server,
        5. read incoming messages from the server

        - whenever a command is received, either from
        the user or from the server, the client app 
        will perform the required actions and display 
        the results to the user */

    while (1) {

        /* monitoring of input events is achieved with
            I/O multiplexing. the client app tracks two
            types of input events: keyboard events
            and socket events. poll() waits for input
            events on stdin and socket file descriptors
            and returns when an event was detected */

        int fdsReady = poll(get_fds(appState.tcpClient), PFDS_COUNT, -1);

        if (fdsReady < 0) {

            /* repaints windows if poll() was interrupted
                by SIGWINCH signal (i.e. window was resized) */

            if (errno == EINTR) {

                if (get_resized()) {

                    repaint_ui(appState.windowManager, appState.scrollback, appState.lnEditor, appState.options.color);
                    set_resized(0);
                }
            }
            else {
                FAILED("Error polling descriptors", NO_ERRCODE);  
            }
        }

        // check for keyboard events
        if (is_stdin_event(appState.tcpClient)) {

            /* reads char from the terminal buffer and 
                executes appropriate action - if control 
                char was read (e.g. arrow, backspace etc.),
                the control action will be performed, if regular
                char was read, it will be added to the buffer 
                and displayed on the screen, if new line was read, 
                the input line will be parsed */

            int ch, index;

            ch = wgetch(get_inputwin(appState.windowManager));
            ch = remap_ctrl_key(ch);

            if ((index = get_sb_func_index(ch)) != -1) {
                get_scrollback_function(index)(appState.scrollback);
            }
            else if ((index = get_le_func_index(ch)) != -1) {
                get_lneditor_function(index)(appState.lnEditor);
            }
            else if (ch == KEY_NEWLINE) {
                parse_input(appState.lnEditor, appState.cmdTokens);
            }           
            else if (isprint(ch)) {
                LOG(INFO, "%d", ch);
                add_char(appState.lnEditor, ch);
            }

            if (appState.cmdTokens->command != NULL) {

                /* checks which command was parsed and
                 executes the appropriate action */
                CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(appState.cmdTokens));
                CommandFunction cmdFunction = get_command_function(commandType);

                cmdFunction(appState.scrollback, appState.tcpClient, appState.cmdTokens);

                reset_cmd_tokens(appState.cmdTokens);
            }
    
            wrefresh(get_inputwin(appState.windowManager));
        }

        /* checks connection status and displays
            the status message */
        if (is_client_connected(appState.tcpClient)) {

            display_status(appState.windowManager, "[%s]  [%s]", get_property_value(NICKNAME), get_server_name(appState.tcpClient));
        }
        else {
            display_status(appState.windowManager, "");
        }

        /* sends outbound messages to the server */
        while (!is_queue_empty(get_client_queue(appState.tcpClient))) {

            RegMessage *message = remove_message_from_client_queue(appState.tcpClient);
            char *content = get_reg_message_content(message);

            client_write(content, get_socket_fd(appState.tcpClient));
        } 

        /* checks for socket events */
        if (is_socket_event(appState.tcpClient)) {

            /* reads data from the socket and displays
                received messages on the screen */
            int fullMsg = client_read(appState.tcpClient);

            if (fullMsg) {

                PrintTokens *printTokens = create_print_tokens(1, " <> ", NULL, get_client_inbuffer(appState.tcpClient));

                printmsg(appState.scrollback, printTokens, COLOR_SEP(RED));
                wrefresh(get_chatwin(appState.windowManager));
                wrefresh(get_inputwin(appState.windowManager));

                delete_print_tokens(printTokens);
            }
        }
    }
	return 0;
}


/* releases memory allocated on the
     heap */
static void cleanup(void) {

    delete_command_tokens(appState.cmdTokens);
    write_settings(NULL, CLIENT_PROPERTY);
    delete_line_editor(appState.lnEditor);
    delete_scrollback(appState.scrollback);
    delete_windows(appState.windowManager);
    delete_client(appState.tcpClient);
    delete_logger(appState.logger);
}

/* reads command line arguments*/
static void get_options(int argc, char **argv, ClientOptions *options) {

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
                options->color = 0;
                break;
            case 'l':
                if (optarg && is_valid_log_level(string_to_log_level(optarg))) {
                    options->logLevel = string_to_log_level(optarg);
                }
                break;
            case 's':
                if (optarg && (str_to_uint(optarg) >= 1 || str_to_uint(optarg) <= 5)) {
                    options->scrollback = str_to_uint(optarg);
                }
                break;
            default:
                printf("Usage: %s [--nocolor] [--scrollback <1-5>] [--loglevel <debug | info | warning | error>]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

#endif


