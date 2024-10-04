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

#define PFDS_COUNT 2

typedef struct {
    Logger *logger;
    TCPClient *tcpClient;
    WindowManager *windowManager;
    Scrollback *scrollback;
    LineEditor *lnEditor;
    CommandTokens *cmdTokens;
} AppState;

#ifndef TEST

static AppState appState = {NULL};

static void cleanup(void);

int main(void)
{
    // register cleanup function
    atexit(cleanup);

    // set signal handler for SIGINT
    set_sigaction(handle_sigint, SIGINT);

    // create logger
    appState.logger = create_logger(NULL, LOG_FILE(client), DEBUG);
    set_stdout_allowed(0);

    LOG(INFO, "Client started");

    // load settings
    set_default_settings();
    read_settings(NULL, CLIENT_PROPERTY);

    // create client tcpClient
    appState.tcpClient = create_client();

    // create ncurses windows and set options
    appState.windowManager = create_windows();
    set_windows_options(appState.windowManager);
    init_colors();

    // create scrollback and line editor
    appState.scrollback = create_scrollback(get_chatwin(appState.windowManager), 0);
    appState.lnEditor = create_line_editor(get_inputwin(appState.windowManager));
    create_layout(appState.windowManager, appState.scrollback);

    // track stdin and socket descriptors for input events
    // struct pollfd pfds[PFDS_COUNT] = {
    //     {.fd = fileno(stdin), .events = POLLIN},
    //     {.fd = -1, .events = POLLIN}
    // };

    appState.cmdTokens = create_command_tokens();

    while (1) {

        int fdsReady = poll(get_fds(appState.tcpClient), PFDS_COUNT, -1);
        if (fdsReady < 0) {
            FAILED("Error polling descriptors", NO_ERRCODE);  
        }

        // check for stdin events
        if (is_stdin_event(appState.tcpClient)) {

            // handle char input
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
            else {
                add_char(appState.lnEditor, ch);
            }

            if (appState.cmdTokens->command != NULL) {

                CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(appState.cmdTokens));
                CommandFunction cmdFunction = get_command_function(commandType);

                cmdFunction(appState.scrollback, appState.tcpClient, appState.cmdTokens);

                reset_cmd_tokens(appState.cmdTokens);
            }
            wrefresh(get_inputwin(appState.windowManager));
        }

        // display or hide connection status
        if (is_client_connected(appState.tcpClient)) {

            display_status(appState.windowManager, "[%s]  [%s]", get_property_value(NICKNAME), get_server_name(appState.tcpClient));
        }
        else {
            display_status(appState.windowManager, "");
        }

        // send messages to the server
        while (!is_queue_empty(get_client_queue(appState.tcpClient))) {

            RegMessage *message = remove_message_from_client_queue(appState.tcpClient);
            char *content = get_reg_message_content(message);

            client_write(content, get_socket_fd(appState.tcpClient));
        } 

        // check for socket input events
        if (is_socket_event(appState.tcpClient)) {

            int fullMsg = client_read(appState.tcpClient);

            if (fullMsg) {

                MessageParams *messageParams = create_message_params(1, " <> ", NULL, get_client_inbuffer(appState.tcpClient));

                printmsg(appState.scrollback, messageParams, COLOR_SEP(RED));
                wrefresh(get_chatwin(appState.windowManager));
                wrefresh(get_inputwin(appState.windowManager));

                free(messageParams);
            }
        }
    }

	return 0;
}

static void cleanup(void) {

    delete_command_tokens(appState.cmdTokens);

    write_settings(NULL, CLIENT_PROPERTY);

    delete_client(appState.tcpClient);

    delete_line_editor(appState.lnEditor);
    delete_scrollback(appState.scrollback);
    delete_windows(appState.windowManager);

    delete_logger(appState.logger);
}

#endif


