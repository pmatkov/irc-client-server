#include "settings.h"
#include "tcpclient.h"
#include "display.h"
#include "scrollback.h"
#include "line_editor.h"
#include "command_handler.h"

#include "../../shared/src/command.h"
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
    Settings *settings;
    TCPClient *tcpClient;
    WindowManager *windowManager;
    Scrollback *scrollback;
    LineEditor *lnEditor;
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
    appState.settings = create_settings();
    set_default_settings(appState.settings);
    read_settings(appState.settings, NULL);

    // create client tcpClient
    appState.tcpClient = create_client();

    // create ncurses windows and set options
    appState.windowManager = create_windows();

    // create scrollback and line editor
    appState.scrollback = create_scrollback(get_chatwin(appState.windowManager), 0);
    appState.lnEditor = create_line_editor(get_inputwin(appState.windowManager));

    set_windows_options(appState.windowManager);
    init_colors(appState.settings);
    create_layout(appState.windowManager, appState.scrollback, appState.settings);

    // track stdin and socket descriptors for input events
    struct pollfd pfds[PFDS_COUNT] = {
        {.fd = fileno(stdin), .events = POLLIN},
        {.fd = -1, .events = POLLIN}
    };

    while (1) {

        int fdsReady = poll(pfds, PFDS_COUNT, -1);
        if (fdsReady < 0) {
            FAILED("Error polling descriptors", NO_ERRCODE);  
        }

        // check for stdin events
        if (pfds[0].revents & POLLIN) {

            // handle char input
            int ch, index;
            CommandTokens *cmdTokens = create_command_tokens();

            ch = wgetch(get_inputwin(appState.windowManager));
            ch = remap_ctrl_key(ch);

            if ((index = get_sb_func_index(ch)) != -1) {
                use_scrollback_func(index)(appState.scrollback);
            }
            else if ((index = get_le_func_index(ch)) != -1) {
                use_line_editor_func(index)(appState.lnEditor);
            }
            else if (ch == KEY_NEWLINE) {
                parse_input(appState.lnEditor, cmdTokens);
            }
            else {
                add_char(appState.lnEditor, ch);
            }

            CommandType commandType = string_to_command_type(get_cmd_from_cmd_tokens(cmdTokens));

            if (commandType != UNKNOWN_COMMAND_TYPE) {

                CommandFunction cmdFunction = get_command_function(commandType);
                cmdFunction(appState.scrollback, appState.settings, appState.tcpClient, cmdTokens);
            }

            delete_command_tokens(cmdTokens);
            wrefresh(get_inputwin(appState.windowManager));

        }

        // check connection
        if (pfds[1].fd == -1 && client_is_connected(appState.tcpClient)) {

            pfds[1].fd = client_get_fd(appState.tcpClient);

            display_status(appState.windowManager, "[%s]  [%s]", get_property_value(appState.settings, NICKNAME), client_get_servername(appState.tcpClient));
        }
        else if (pfds[1].fd != -1 && !client_is_connected(appState.tcpClient)) {
            pfds[1].fd = -1;
        }

        // send messages
        if (!is_queue_empty(client_get_queue(appState.tcpClient))) {

            client_write(appState.tcpClient);
        }

        // check for socket input events
        if (pfds[1].revents & POLLIN) {

            int fullMsg = client_read(appState.tcpClient);

            if (fullMsg) {

                MessageParams *messageParams = create_message_params(1, " <> ", NULL, client_get_buffer(appState.tcpClient));

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

    write_settings(appState.settings, NULL);
    delete_settings(appState.settings);

    delete_client(appState.tcpClient);

    delete_line_editor(appState.lnEditor);
    delete_scrollback(appState.scrollback);
    delete_windows(appState.windowManager);

    delete_logger(appState.logger);
}

#endif


