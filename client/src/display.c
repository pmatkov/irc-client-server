#ifdef TEST
#include "priv_display.h"
#include "priv_scrollback_window.h"
#include "../../libs/src/mock.h"
#else
#include "display.h"
#include "scrollback_window.h"
#endif

#include "print_manager.h"
#include "input_controller.h"
#include "scroll_observer.h"
#include "../../libs/src/common.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/time_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <errno.h>
#include <wchar.h>
#include <stdarg.h>

#define NCURSES_WIDECHAR 1
#include <ncursesw/curses.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define TIMESTAMP_WIDTH 8
#define MARKER_COUNT 2

#ifndef TEST

struct WindowManager {
    WINDOW *stdscr;
    BaseWindow *titlewin;
    WindowGroup *mainWindows;
    BaseWindow *statuswin;
    InputWindow *inputwin;
    ScrollObserver *observer;
};

#endif

STATIC void create_window_borders(WindowManager *windowManager, int useColor);
STATIC void draw_border(WINDOW *window, int y, int x, int width);
STATIC void set_status_params(WINDOW *window, StatusParams *statusParams);
STATIC void update_status_message(void *instance, const char *message);
STATIC void display_list_item(const char *string, void *arg);

WindowManager * create_window_manager(int sbMultiplier, int cmdHistoryCount) {

    WindowManager *windowManager = (WindowManager*) malloc(sizeof(WindowManager));
    if (windowManager == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    /* set locale to the locale of current environment */
    setlocale(LC_ALL, "");

    /* initialize ncurses data structures. when 
        initscr is called successfully, it sets 
        the variables stdscr and curscr, which 
        are only non-null after initialization */
    windowManager->stdscr = initscr(); 

    int rows = get_wheight(windowManager->stdscr);
    int cols = get_wwidth(windowManager->stdscr);

    /*  the UI consists of several windows: 
    1. the main ncurses window "stdscr" is container for
        other windows, 
    2. the "title" window display the title,
    3. one or more "main" windows display user's commands, 
        client's responses and chat messages,
    4. the "status" window displays status messages,
    5. the "input" window displays user input */

    windowManager->mainWindows = create_window_group(0);
    ScrollbackWindow *sbWindow = create_scrollback_window(rows - 3, cols, 1, 0, sbMultiplier);
    add_window(windowManager->mainWindows, (BaseWindow*)sbWindow);

    windowManager->titlewin = create_base_window(1, cols, 0, 0, BASE_WINDOW);
    windowManager->statuswin = create_base_window(1, cols, rows - 2, 0, BASE_WINDOW);
    windowManager->inputwin = create_input_window(1, cols, rows - 1, 0, cmdHistoryCount); 

    windowManager->observer = create_scroll_observer(get_scroll_subject(get_scrollback(sbWindow)), update_status_message, windowManager->statuswin, windowManager->inputwin);

    return windowManager;
}

void delete_window_manager(WindowManager *windowManager) {

    if (windowManager != NULL) { 

        endwin();
        delete_input_window(windowManager->inputwin);
        delete_base_window(windowManager->statuswin);
        delete_window_group(windowManager->mainWindows);
        delete_base_window(windowManager->titlewin);

    }
    free(windowManager);
}

void set_windows_options(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* disable line buffering and character echo */
    cbreak();
    noecho(); 

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    /* capture special keys */
    keypad(get_window(inputBaseWindow), TRUE);
    /* enable scrolling */
    scrollok(get_window(mainBaseWindow), TRUE);     
    /* optimize scrolling with escape sequences */
    idlok(get_window(mainBaseWindow), TRUE);
}

void init_colors(int useColor) {

    if (useColor && has_colors()) {

        /* define ncurses colors pairs */
        start_color();

        init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(RED, COLOR_RED, COLOR_BLACK);
        init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(CYAN_REV, COLOR_BLACK, COLOR_CYAN);
        init_pair(RED_CYAN, COLOR_RED, COLOR_CYAN);
    }
}

void init_ui(WindowManager *windowManager, int useColor) {  

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    create_window_borders(windowManager, useColor);

    /* display default app info */
    print_tokens(mainBaseWindow, &(MessageTokens){1, " ## ", NULL, "Buzz v1.0", COLOR_SEP(MAGENTA) | STYLE_CNT(BOLD)});

    /* display active settings */
    display_settings(windowManager);

    print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
    print_tokens(mainBaseWindow, &(MessageTokens){1, " ** ", NULL, "Type /help for a list of available commands.", COLOR_SEP(CYAN)});

    wrefresh(get_window(mainBaseWindow));

    display_time_status(windowManager);

    /* set input prompt */
    mvwaddstr(get_window(inputBaseWindow), 0, 0, PROMPT);
    wrefresh(get_window(inputBaseWindow));
}

/* the top border contains the title and the
    bottom border is created inside the "status
    window" */
STATIC void create_window_borders(WindowManager *windowManager, int useColor) {
    
    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    draw_border(get_window(windowManager->titlewin), 0, 0, 0);

    int attribute = useColor ? COLOR_PAIR(CYAN_REV) : A_REVERSE;

    wattron(get_window(windowManager->titlewin), attribute);
    mvwaddstr(get_window(windowManager->titlewin), 0, 1, "Buzz - IRC client");
    wattroff(get_window(windowManager->titlewin), attribute); 

    wrefresh(get_window(windowManager->titlewin));

    draw_border(get_window(windowManager->statuswin), 0, 0, 0);
    wrefresh(get_window(windowManager->statuswin));
}

STATIC void draw_border(WINDOW *window, int y, int x, int width) {

    if (window == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    if (!width) {
        width = get_wwidth(window);
    }

    mvwhline_set(window, y, x, &block, width);
}

void display_commands(WindowManager *windowManager, const CommandInfo **commandInfos, int count) {

    if (windowManager == NULL || commandInfos == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
    print_tokens(mainBaseWindow, &(MessageTokens){1, " ** ", NULL, "Commands:", COLOR_SEP(CYAN)});

    while (*commandInfos != NULL) {

        print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE, NULL, get_cmd_info_label(*commandInfos++), STYLE_CNT(DIM)});
    }

    print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
    print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE, NULL, "Type /help <command name> for usage details.", 0});

    wrefresh(get_window(mainBaseWindow));
    wrefresh(get_window(inputBaseWindow));

}

void display_usage(WindowManager *windowManager, const CommandInfo *commandInfo) {

    if (windowManager == NULL || commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
    print_tokens(mainBaseWindow, &(MessageTokens){1, " ** ", "CommandInfo ", get_cmd_info_label(commandInfo), COLOR_SEP(CYAN) | STYLE_CNT(DIM)});

    print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE, "Syntax:", NULL, 0});
    print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE_2, NULL, get_cmd_info_syntax(commandInfo), 0});

    print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE, NULL, "Description:", 0});
    iterate_string_list(get_cmd_info_description(commandInfo), MAX_TOKENS, display_list_item, windowManager);
    print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE, NULL, "Example(s):", 0});
    iterate_string_list(get_cmd_info_examples(commandInfo), MAX_TOKENS, display_list_item, windowManager);

    wrefresh(get_window(mainBaseWindow));
    wrefresh(get_window(inputBaseWindow));
}

void display_response(WindowManager *windowManager, const char *response, ...) {

    if (windowManager == NULL || response == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    if (strchr(response, '%') == NULL) {

        print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
        print_tokens(mainBaseWindow, &(MessageTokens){1, " ** ", NULL, response, COLOR_SEP(CYAN) | STYLE_CNT(BOLD)});
    }
    else {
        char responseWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, response);
        vsnprintf(responseWithArgs, MAX_CHARS, response, arglist);
        va_end(arglist);

        print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
        print_tokens(mainBaseWindow, &(MessageTokens){1, " ** ", NULL, responseWithArgs, COLOR_SEP(CYAN) | STYLE_CNT(BOLD)});

    }
    wrefresh(get_window(mainBaseWindow));
    wrefresh(get_window(inputBaseWindow));
}

void display_server_message(const char *string, void *arg) {

    if (string == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *mainBaseWindow = get_active_window(((WindowManager*)arg)->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(((WindowManager*)arg)->inputwin);

    print_tokens(mainBaseWindow, &(MessageTokens){1, " >> ", NULL, string, COLOR_SEP(RED)});

    wrefresh(get_window(mainBaseWindow));
    wrefresh(get_window(inputBaseWindow));
}

void display_settings(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    print_tokens(mainBaseWindow, &(MessageTokens){1, NULL, NULL, NULL, 0});
    print_tokens(mainBaseWindow, &(MessageTokens){1, " ** ", NULL, "Assigned settings: ", COLOR_SEP(CYAN)});

    char propertyValue[MAX_CHARS + 1] = {'\0'};

    for (int i = 0; i < get_settings_capacity(); i++) {

        if (is_option_registered(i)) {

            memset(propertyValue, '\0', ARRAY_SIZE(propertyValue));
            propertyValue[0] = ' ';

            if (get_option_data_type(i) == INT_TYPE) {

                char valueStr[MAX_CHARS + 1] = {'\0'};

                int value = get_int_option_value(i);
                uint_to_str(valueStr, ARRAY_SIZE(valueStr), value);

                strcat(propertyValue, valueStr); 
            }
            else if (get_option_data_type(i) == CHAR_TYPE) {
                strcat(propertyValue, get_char_option_value(i));
            }

            print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE, get_option_label(i), propertyValue, STYLE_CNT(DIM)});
        }
    }

    wrefresh(get_window(mainBaseWindow));
    wrefresh(get_window(inputBaseWindow));
}

void display_time_status(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    char timestamp[HM_TIME_LENGTH] = {'\0'};
    get_datetime(get_format_function(HM_TIME), timestamp, HM_TIME_LENGTH);

    uint32_t format = COLOR_SEP(RED_CYAN) | COLOR_ORG(CYAN_REV) | COLOR_CNT(RED_CYAN) | STYLE_SEP(DIM) | STYLE_CNT(DIM);

    display_status(windowManager->statuswin, windowManager->inputwin, &(StatusParams){TIME_STATUS, 0, 0, 0, {"[", "]"}, timestamp, format});
    wrefresh(get_window(windowManager->statuswin));
    wrefresh(get_window(inputBaseWindow));
}

void display_status(BaseWindow *statusWindow, InputWindow *inputWindow, StatusParams *statusParams, ...) {

    if (statusWindow == NULL || inputWindow == NULL || statusParams == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char statusWithArgs[MAX_CHARS] = {'\0'};

    if (strchr(statusParams->message, '%') != NULL) {

        va_list arglist;
        va_start(arglist, statusParams);
        vsnprintf(statusWithArgs, MAX_CHARS, statusParams->message, arglist);
        va_end(arglist);

        statusParams->message = statusWithArgs;
    }

    if (!statusParams->format) {
        statusParams->format = COLOR_SEP(CYAN_REV) | COLOR_ORG(CYAN_REV) | COLOR_CNT(CYAN_REV);
    }

    BaseWindow *inputBaseWindow = get_le_base_window(inputWindow);

    set_status_params(get_window(inputBaseWindow), statusParams);

    draw_border(get_window(statusWindow), 0, statusParams->fieldStart, statusParams->fieldWidth);

    print_tokens_xy(get_window(statusWindow), &(MessageTokens){0, statusParams->marker[0], statusParams->message, statusParams->marker[1], statusParams->format}, 0, statusParams->textStart);

    wrefresh(get_window(statusWindow));
    wrefresh(get_window(get_le_base_window(inputWindow)));
}

STATIC void set_status_params(WINDOW *window, StatusParams *statusParams) {

    if (window == NULL || statusParams == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int cols = get_wwidth(window);

    if (statusParams->statusType == TIME_STATUS) {
        statusParams->textStart = 1;
        statusParams->fieldStart = statusParams->textStart;
        statusParams->fieldWidth = TIMESTAMP_WIDTH;

    }
    else if (statusParams->statusType == MAIN_STATUS) {
        statusParams->textStart = TIMESTAMP_WIDTH + 1;
        statusParams->fieldStart = statusParams->textStart;
        statusParams->fieldWidth = cols/ 2 - statusParams->textStart;
    }
    else if (statusParams->statusType == SIDE_STATUS) {
        statusParams->textStart = cols - strlen(statusParams->message) - strlen(statusParams->marker[0]) - strlen(statusParams->marker[1]) - 1;
        statusParams->fieldStart = cols/ 2;
        statusParams->fieldWidth = cols/ 2;
    }

    if (strcmp(statusParams->message, "") == 0) {
        statusParams->marker[0] = "";
        statusParams->marker[1] = "";
    }
}

STATIC void update_status_message(void *instance, const char *message) {

    if (instance == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    ScrollObserver *scrollObserver = instance;

    display_status(get_ob_status_window(scrollObserver), get_ob_input_window(scrollObserver), &(StatusParams){SIDE_STATUS, 0, 0, 0, {"-", "-"}, message, 0});
}

STATIC void display_list_item(const char *string, void *arg) {

    BaseWindow *mainBaseWindow = get_active_window(((WindowManager*)arg)->mainWindows);

    print_tokens(mainBaseWindow, &(MessageTokens){1, SPACE_2, NULL, string, 0});
}

void resize_ui(WindowManager *windowManager, int useColors) {

    BaseWindow *mainBaseWindow = get_active_window(windowManager->mainWindows);
    BaseWindow *inputBaseWindow = get_le_base_window(windowManager->inputwin);

    const int MIN_ROWS = 4;

    /* temporarily exit ncurses mode and refresh to 
        reinitialize the terminal with the new size.
        after resizing, clean the window structures 
        and reinitialize the keypad */
    endwin();
    refresh();
    wclear(get_window(mainBaseWindow));
    wclear(get_window(windowManager->statuswin));
    wclear(get_window(inputBaseWindow));
    keypad(get_window(inputBaseWindow), TRUE);

    int rows = get_wheight(windowManager->stdscr);
    int cols = get_wwidth(windowManager->stdscr);

    /* hide the cursor if the terminal is smaller
        than the minimum size required to display the 
        "input window" */
    if (rows < MIN_ROWS && curs_set(0)) {
        curs_set(0);
    }
    else if (rows >= MIN_ROWS && !curs_set(1)) {
        curs_set(1);
    }
    if (rows > 1) {

        int rowsMainWin = rows < MIN_ROWS ? 1 : rows - 3;

        wresize(get_window(mainBaseWindow), rowsMainWin, cols);
        mvwin(get_window(mainBaseWindow), 1, 0);
    }
    if (rows > 2) {

        int y = rows > 3 ? rows - 2 : rows - 1;

        wresize(get_window(windowManager->statuswin), 1, cols);
        mvwin(get_window(windowManager->statuswin), y, 0);
    }
    if (rows > 3) {

        wresize(get_window(inputBaseWindow), 1, cols);
        mvwin(get_window(inputBaseWindow), rows - 1, 0);
    }

    /* redraw window borders, reload the content from 
        the scrollback and display the last command */
    create_window_borders(windowManager, useColors);
    restore_from_scrollback(mainBaseWindow);

    mvwaddstr(get_window(inputBaseWindow), 0, 0, PROMPT);
    display_current_command(inputBaseWindow);

    wrefresh(get_window(inputBaseWindow));
}

BaseWindow * get_title_window(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->titlewin;
}

WindowGroup * get_main_windows(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->mainWindows;
}

BaseWindow * get_status_window(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->statuswin;
}

InputWindow * get_input_window(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->inputwin;
}