#include "main.h"
#include "tcp_client.h"
#include "display.h"
#include "scrollback.h"
#include "line_editor.h"
#include "command_handler.h"
#include "../../libs/src/lookup_table.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
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

#define PFDS_COUNT 2

/* clientOptions contains command line
    options */
typedef struct {
    int color;
    int sbMultiplier;
    LogLevel logLevel;
} ClientOptions;

/* appStat holds references to heap
    allocated data for cleanup purposes */
typedef struct {
    Logger *logger;
    LookupTable *lookupTable;
    Settings *settings;
    TCPClient *tcpClient;
    WindowManager *windowManager;
    // Scrollback *scrollback;
    // LineEditor *lnEditor;
    CommandTokens *cmdTokens;
} AppState;


#ifndef TEST

static AppState appState = {NULL};

static void get_options(int argc, char **argv, ClientOptions *options);
static void cleanup(void);

int main(int argc, char **argv)
{
    /* register the cleanup function */
    atexit(cleanup);

    /* set signal handlers for SIGINT (program 
        interrupt) and SIGWINCH (resize
        event) signals */
    set_sigaction(handle_sigint, SIGINT);
    set_sigaction(handle_sigwinch, SIGWINCH);

    /* set default values for the client's options */
    ClientOptions options = {.color = 1, .sbMultiplier = 5, .logLevel = DEBUG};

    /* parse command line arguments */
    get_options(argc, argv, &options);

    /* create logger and set logging options */
    appState.logger = create_logger(NULL, LOG_FILE(client), options.logLevel);
    set_stdout_allowed(0);

    LOG(INFO, "Client started");

    /* keys and labels are lookup tables for 
        settings' properties */
    int keys[] = {CP_NICKNAME, CP_USERNAME, CP_REALNAME, CP_ADDRESS, CP_PORT};
    const char *values[] = {"nickname", "username", "realname", "address", "port"};
    static_assert(ARR_SIZE(keys) == ARR_SIZE(values), "Array size mismatch");

    appState.lookupTable = create_lookup_table(keys, values, ARR_SIZE(keys));
    appState.settings = create_settings(ARR_SIZE(keys));

    initialize_settings(appState.settings, appState.lookupTable);

    /* tcpClient provides networking 
        functionality for the app */
    appState.tcpClient = create_client();

    /* create ncurses windows, set options and
        initialize colors */
    appState.windowManager = create_windows(options.sbMultiplier);
    set_windows_options(appState.windowManager);
    init_colors(options.color);

    /* create scrollback and line editor */
    // appState.scrollback = create_scrollback(get_chatwin(appState.windowManager), options.sbMultiplier);
    // appState.lnEditor = create_line_editor(get_inputwin(appState.windowManager));

    /* initialize user interface */
    init_ui(appState.windowManager, options.color);

    /* command tokens contain parsed command
        tokens */
    appState.cmdTokens = create_command_tokens();

    /* the basic program flow includes the 
        following actions:

        1. read keyboard input,
        2. parse input,
        3.a if input is a local command, execute it,
        3.b if input is an IRC command, transform it, if 
            required, and add it to the message queue,
        4.a establish a connection to the server, if not
            already established,
        4.b send messages to the server,
        5. read incoming server messages and execute the
            appropriate action */

    while (1) {

        /* poll() is used to monitor input events on the keyboard
            and network socket. Monitoring is done via a set 
            of poll fd's that represent communication interfaces */
        int fdsReady = poll(get_fds(appState.tcpClient), PFDS_COUNT, -1);

        if (fdsReady < 0) {

            /* repaint UI if poll() was interrupted by 
                SIGWINCH signal (i.e., the window was 
                resized) */
            if (errno == EINTR) {

                if (get_resized()) {

                    resize_ui(appState.windowManager, options.color);
                    set_resized(0);
                }
            }
            else {
                FAILED("Error polling descriptors", NO_ERRCODE);  
            }
        }

        /* check for keyboard events */
        if (is_stdin_event(appState.tcpClient)) {

            /* read char from the terminal and execute the
                appropriate action - if a control char was
                read (e.g. arrow, backspace etc.), the control
                action will be executed, if a regular char 
                was read, it will be added to the buffer 
                and displayed on the screen, if a newline char 
                was read, the input will be parsed */
            int ch, index;

            ch = wgetch(get_window(get_inputwin(appState.windowManager)));
            ch = remap_ctrl_key(ch);

            if ((index = get_sb_func_index(ch)) != -1) {
                get_scrollback_function(index)(get_scrollback(get_chatwin(appState.windowManager)));
            }
            else if ((index = get_le_func_index(ch)) != -1) {
                get_lneditor_function(index)(get_line_editor(get_inputwin(appState.windowManager)));
            }
            else if (ch == KEY_NEWLINE) {
                parse_input(get_line_editor(get_inputwin(appState.windowManager)), appState.cmdTokens);
            }           
            else if (isprint(ch)) {
                add_char(get_line_editor(get_inputwin(appState.windowManager)), ch);
            }

            if (appState.cmdTokens->command != NULL) {

                /* check if a valid command was parsed,
                    and if so, execute it */
                CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(appState.cmdTokens));
                CommandFunc commandFunc = get_command_function(commandType);

                commandFunc(appState.windowManager, appState.tcpClient, appState.cmdTokens);

                reset_cmd_tokens(appState.cmdTokens);
            }
    
            wrefresh(get_window(get_inputwin(appState.windowManager)));
        }

        /* send messages to the server */
        while (!is_queue_empty(get_client_queue(appState.tcpClient))) {

            RegMessage *message = remove_message_from_client_queue(appState.tcpClient);
            char *content = get_reg_message_content(message);

            client_write(content, get_socket_fd(appState.tcpClient));
        } 

        /* check for socket events */
        if (is_socket_event(appState.tcpClient)) {

            /* read data from the socket and display
                received messages */
            int fullMsg = client_read(appState.tcpClient);

            if (fullMsg) {

                PrintTokens *printTokens = create_print_tokens(1, " <> ", NULL, get_client_inbuffer(appState.tcpClient), COLOR_SEP(RED));

                printstr(printTokens, appState.windowManager);
                wrefresh(get_window(get_chatwin(appState.windowManager)));
                wrefresh(get_window(get_inputwin(appState.windowManager)));

                delete_print_tokens(printTokens);
            }
        }
    }
	return 0;
}

/* get command line options */
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
                    options->sbMultiplier = str_to_uint(optarg);
                }
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

/*  write settings and release heap allocated
    memory */
static void cleanup(void) {

    write_settings(NULL);

    delete_command_tokens(appState.cmdTokens);
    // delete_line_editor(appState.lnEditor);
    // delete_scrollback(appState.scrollback);
    delete_windows(appState.windowManager);
    delete_client(appState.tcpClient);
    delete_settings(appState.settings);
    delete_lookup_table(appState.lookupTable);
    delete_logger(appState.logger);
}

#endif

void initialize_settings(Settings *settings, LookupTable *lookupTable) {

    if (settings == NULL || lookupTable == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    /* obtain username of currently active user */
    struct passwd *userRecord = getpwuid(getuid());
    char *name = userRecord != NULL ? userRecord->pw_name : "";

    register_property(CHAR_TYPE, get_lookup_pair(lookupTable, CP_NICKNAME), name);
    register_property(CHAR_TYPE, get_lookup_pair(lookupTable, CP_USERNAME), name);
    register_property(CHAR_TYPE, get_lookup_pair(lookupTable, CP_REALNAME), "anonymous");
    register_property(CHAR_TYPE, get_lookup_pair(lookupTable, CP_ADDRESS), "localhost");
    register_property(CHAR_TYPE, get_lookup_pair(lookupTable, CP_PORT), "50100");

    read_settings(lookupTable, NULL);

}
