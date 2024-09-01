#include "settings.h"
#include "session.h"
#include "display.h"
#include "scrollback.h"
#include "line_editor.h"
#include "../../shared/src/parser.h"
#include "../../shared/src/signal_handler.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>

#ifdef TEST
    #define LOG_FILE(str) "test_" #str
#else
    #define LOG_FILE(str) #str
#endif

#define PFDS_COUNT 2

typedef struct {
    Logger *logger;
    Settings *settings;
    Session *session;
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
    appState.logger = create_logger(NULL, LOG_FILE(client));
    set_stdout_allowed(0);

    LOG(INFO, "Client started");

    // load settings
    appState.settings = create_settings();
    set_default_settings(appState.settings);
    read_settings(appState.settings, "data/buzz.conf");

    // create client session
    appState.session = create_session();

    // create ncurses windows and set options
    appState.windowManager = create_windows();

    // create scrollback and line editor
    appState.scrollback = create_scrollback(get_chatwin(appState.windowManager), 0);
    appState.lnEditor = create_line_editor(get_inputwin(appState.windowManager));

    // RegMessage message;
    // set_reg_message(&message, "");
    // enqueue(get_message_queue(appState.lnEditor), &message);

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

            // process char input
            int ch = wgetch(get_inputwin(appState.windowManager));

            const char *keystr = keyname(ch);

            if (keystr != NULL && strcmp(keystr, "kUP5") == 0) {
                scroll_line_up(appState.scrollback);
            }
            else if (keystr != NULL && strcmp(keystr, "kDN5") == 0) {
                scroll_line_down(appState.scrollback);
            }
            else {
            
                switch(ch) {

                    case KEY_LEFT:
                        move_cursor_left(appState.lnEditor);
                        break;
                    case KEY_RIGHT:
                        move_cursor_right(appState.lnEditor);
                        break;

                    case KEY_UP:
                        // scroll_line_up(appState.scrollback);
                        display_command_history(appState.lnEditor, 1);
                        break;
                    case KEY_DOWN:
                        display_command_history(appState.lnEditor, -1);
                        break;

                    case KEY_PPAGE:
                        scroll_page_up(appState.scrollback);
                        break;
                    case KEY_NPAGE:
                        scroll_page_down(appState.scrollback);
                        break;
                    case KEY_BACKSPACE:
                        use_backspace(appState.lnEditor);
                        break;
                    case KEY_DC:
                        use_delete(appState.lnEditor);
                        break;
                    case KEY_HOME:
                        use_home(appState.lnEditor);
                        break;
                    case KEY_END:
                        use_end(appState.lnEditor);
                        break;
                    case KEY_NEWLINE:
                        parse_input(appState.lnEditor, appState.scrollback, appState.settings, appState.session);
                        break;
                    case KEY_RESIZE:   
                        handle_resize(appState.windowManager, appState.scrollback);
                        break;
                    default:
                        add_char(appState.lnEditor, ch);
                }
            }        

            wrefresh(get_inputwin(appState.windowManager));
        }

        if (pfds[1].fd == -1 && session_is_connected(appState.session)) {
            pfds[1].fd = session_get_fd(appState.session);
        }
        else if (pfds[1].fd != -1 && !session_is_connected(appState.session)) {
            pfds[1].fd = -1;
        }

        // if messages in buffer, send

  

        // check for socket event

        if (pfds[1].revents & POLLIN) {


        }


       

    }

	return 0;
}

static void cleanup(void) {

    write_settings(appState.settings, "data/buzz.conf");
    delete_settings(appState.settings);

    delete_session(appState.session);

    delete_windows(appState.windowManager);
    delete_scrollback(appState.scrollback);
    delete_line_editor(appState.lnEditor);

    delete_logger(appState.logger);
}

#endif


