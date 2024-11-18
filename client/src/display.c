#ifdef TEST
#include "priv_display.h"
#include "../../libs/src/mock.h"
#else
#include "display.h"
#endif

#include "../../libs/src/settings.h"
#include "../../libs/src/time_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <errno.h>

#include <wchar.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_SENTENCES 5
#define STATUS_START_POS 6

#ifndef TEST

struct WindowManager {
    WINDOW *stdscr;
    UIWindow *titlewin;
    UIWindow *chatwin;
    UIWindow *statuswin;
    UIWindow *inputwin;
};

struct PrintTokens {
    int useTimestamp;
    const char *separator;
    const char *origin;
    const char *content;
    uint32_t format;
};

#endif

STATIC void create_window_borders(WindowManager *windowManager, int useColor);
STATIC void draw_border(WINDOW *window, int y, int x, int width);
STATIC void print_complex_string(WindowManager *windowManager, cchar_t *string, int size);
STATIC void display_list_item(const char *string, void *arg);
// STATIC void display_string_list(WindowManager *windowManager, const char **stringList, int count, const char *title);

static PrintFunc printFunc = print_complex_string;

/* ncurses text styles */
static const unsigned ATTR_CONVERT[] = {
    A_NORMAL,
    A_BOLD,
    A_STANDOUT,
    A_DIM,
    A_ITALIC
};

WindowManager * create_windows(int sbMultiplier) {

    WindowManager *windowManager = (WindowManager*) malloc(sizeof(WindowManager));
    if (windowManager == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    /* set locale to that of local environment */
    setlocale(LC_ALL, "");

    /* initialize ncurses data structures */
    windowManager->stdscr = initscr(); 

    int rows = get_wheight(windowManager->stdscr);
    int cols = get_wwidth(windowManager->stdscr);

    /*  the terminal screen is divided into several windows: 
    1. the main terminal window managed by ncurses is "stdscr", 
    2. the top line of "stdscr" is the "title window" that
        contains the top border and the title,
    3. below is the "chat window" which extends up to
        the last two lines of "stdscr". it displays 
        commands, responses and messages,
    4. below the "chat window" is the "status window" that
        contains the bottom border and displays status messages,
    5. at the bottom is the "input window" that displays user 
        input */

    WINDOW *titlewin = newwin(1, cols, 0, 0);
    WINDOW *chatwin = newwin(rows - 3, cols, 1, 0);
    WINDOW *statuswin = newwin(1, cols, rows - 2, 0);
    WINDOW *inputwin = newwin(1, cols, rows - 1, 0);

    windowManager->titlewin = create_ui_window(titlewin, NULL, NO_BACKING);
    windowManager->chatwin = create_ui_window(chatwin, create_scrollback(chatwin, sbMultiplier), SCROLLBACK); 
    windowManager->statuswin = create_ui_window(statuswin, NULL, NO_BACKING);  
    windowManager->inputwin = create_ui_window(inputwin, create_line_editor(inputwin), LINE_EDITOR);

    return windowManager;
}

void delete_windows(WindowManager *windowManager) {

    if (windowManager != NULL) { 

        endwin();
        delete_ui_window(windowManager->inputwin);
        delete_ui_window(windowManager->statuswin);
        delete_ui_window(windowManager->chatwin);
        delete_ui_window(windowManager->titlewin);

    }
    free(windowManager);
}

PrintTokens * create_print_tokens(int useTimestamp, const char *separator, const char *origin, const char *content, uint32_t format) {

    PrintTokens *printTokens = (PrintTokens *) malloc(sizeof(PrintTokens));

    printTokens->useTimestamp = useTimestamp;
    printTokens->separator = separator;
    printTokens->origin = origin;
    printTokens->content = content;
    printTokens->format = format;

    return printTokens;
}

void delete_print_tokens(PrintTokens *printTokens) {

    free(printTokens);
}

void set_windows_options(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* disable line buffering and character echo */
    cbreak();
    noecho(); 

    /* capture special keys */
    keypad(get_window(windowManager->inputwin), TRUE);
    /* enable scrolling */
    scrollok(get_window(windowManager->chatwin), TRUE);     
    /* optimize scrolling with escape sequences */
    idlok(get_window(windowManager->chatwin), TRUE);

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
    }

}

void init_ui(WindowManager *windowManager, int useColor) {  

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    create_window_borders(windowManager, useColor);

    /* display default app info */
    printstr(&(PrintTokens){1, " ## ", NULL, "Buzz v1.0", COLOR_SEP(MAGENTA) | STYLE_CNT(BOLD)}, windowManager);

    /* display active settings */
    display_settings(windowManager);

    printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
    printstr(&(PrintTokens){1, " ** ", NULL, "Type /help for a list of available commands.", COLOR_SEP(CYAN)}, windowManager);

    wrefresh(get_window(windowManager->chatwin));

    display_time(windowManager);

    /* display input prompt */
    mvwaddstr(get_window(windowManager->inputwin), 0, 0, PROMPT);
    wrefresh(get_window(windowManager->inputwin));
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

void printstr(PrintTokens *printTokens, WindowManager *windowManager) {

    if (printTokens == NULL || windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = NULL;

    /* tokens are converted to a cchar_t representation and 
        formatted based on the provided format. the color and 
        style are 4 bit values stored in the last 12 bits of 
        the format field, for the color, and the next 12 bits,
        for the style */
        bufferPtr = buffer;

        if (printTokens->useTimestamp) {

            char timestamp[HM_TIME_LENGTH] = {'\0'};
            get_datetime(get_format_function(HM_TIME), timestamp, HM_TIME_LENGTH);
            
            bufferPtr += string_to_complex_string(bufferPtr, get_remaining_cchars(buffer), timestamp, 0);
        }
        if (printTokens->separator != NULL) {

            bufferPtr += string_to_complex_string(bufferPtr, get_remaining_cchars(buffer), printTokens->separator, COMPRESS_BITS(0, 3, (printTokens->format & BIT_MASK_SEP)));
        }
        if (printTokens->origin != NULL) {

            bufferPtr += string_to_complex_string(bufferPtr, get_remaining_cchars(buffer), printTokens->origin, COMPRESS_BITS(1, 4, (printTokens->format & BIT_MASK_ORG)));
        }
        if (printTokens->content != NULL) {

            bufferPtr += string_to_complex_string(bufferPtr, get_remaining_cchars(buffer), printTokens->content, COMPRESS_BITS(2, 5, (printTokens->format & BIT_MASK_CNT)));
        }

        printFunc(windowManager, buffer, bufferPtr - buffer);

}

STATIC void print_complex_string(WindowManager *windowManager, cchar_t *string, int size) {

    /* add complex string to the scrollback buffer 
        and print it */
    Scrollback *sb = get_scrollback(windowManager->chatwin);
    add_to_scrollback(sb, string, size);
    print_from_scrollback(sb, -1, get_scrollback_head(sb));

}

int string_to_complex_string(cchar_t *buffer, int size, const char *string, uint32_t format) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (string == NULL || !strlen(string)) {
        return 0;
    }

    if (strlen(string) >= size) {
        LOG(ERROR, "Not enough space in the buffer");
        return 0;
    }

    int color = format & 0xF;
    attr_t style = (format >> BITS_PER_HEX) & 0xF;

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};
    int charsConverted = 0;

    /* convert chars to wide chars (wchar_t)
        representation */
    if ((charsConverted = mbstowcs(wstring, string, MAX_CHARS)) == -1) {
        FAILED(NO_ERRCODE, "Error converting char to wchar");
    }

    wchar_t temp[2];
    int i = 0;

    /* convert wide chars to complex chars (cchar_t)
        representation */
    while (i < charsConverted && wstring[i] != L'\0') {

        temp[0] = wstring[i];
        temp[1] = L'\0';  

        if (setcchar(buffer++, temp, ATTR_CONVERT[(Attributes)style], color, NULL) != OK) {
            FAILED(NO_ERRCODE, "Error converting wchar to cchar");
        }
        i++;
    }
    return i;
}

int count_complex_chars(cchar_t *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int i = 0;

    while (string[i].chars[0] != L'\0') {

        i++;
    }
    return i;
}

void display_commands(WindowManager *windowManager, const CommandInfo *commandInfo, int count) {

    if (windowManager == NULL || commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
    printstr(&(PrintTokens){1, " ** ", NULL, "Commands:", COLOR_SEP(CYAN)}, windowManager);

    char *cmd = (char*) commandInfo;
    int size = get_command_info_size();

    for (int i = 1; i < count - 1; i++) {

        printstr(&(PrintTokens){1, SPACE, NULL, get_command_info_label((CommandInfo*)(cmd +  i * size)), STYLE_CNT(DIM)},windowManager);
    }

    printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
    printstr(&(PrintTokens){1, SPACE, NULL, "Type /help <command name> for usage details.", 0}, windowManager);

    wrefresh(get_window(windowManager->chatwin));
    wrefresh(get_window(windowManager->inputwin));

}

void display_usage(WindowManager *windowManager, const CommandInfo *commandInfo) {

    if (windowManager == NULL || commandInfo == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
    printstr(&(PrintTokens){1, " ** ", "CommandInfo ", get_command_info_label(commandInfo), COLOR_SEP(CYAN) | STYLE_CNT(DIM)}, windowManager);

    printstr(&(PrintTokens){1, SPACE, "Syntax:", NULL, 0}, windowManager);
    printstr(&(PrintTokens){1, SPACE_2, NULL, get_command_info_syntax(commandInfo), 0}, windowManager);

    printstr(&(PrintTokens){1, SPACE, NULL, "Description:", 0}, windowManager);
    iterate_string_list(get_command_info_description(commandInfo), MAX_TOKENS, display_list_item, windowManager);
    printstr(&(PrintTokens){1, SPACE, NULL, "Example(s):", 0}, windowManager);
    iterate_string_list(get_command_info_examples(commandInfo), MAX_TOKENS, display_list_item, windowManager);

    wrefresh(get_window(windowManager->chatwin));
    wrefresh(get_window(windowManager->inputwin));
}

void display_response(WindowManager *windowManager, const char *response, ...) {

    if (windowManager == NULL || response == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    else if (strchr(response, '%') == NULL) {

        printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
        printstr(&(PrintTokens){1, " ** ", NULL, response, COLOR_SEP(CYAN) | STYLE_CNT(BOLD)}, windowManager);
    }
    else {
        char responseWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, response);
        vsnprintf(responseWithArgs, MAX_CHARS, response, arglist);
        va_end(arglist);

        printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
        printstr(&(PrintTokens){1, " ** ", NULL, responseWithArgs, COLOR_SEP(CYAN) | STYLE_CNT(BOLD)}, windowManager);

    }
    wrefresh(get_window(windowManager->chatwin));
    wrefresh(get_window(windowManager->inputwin));
}

void display_server_message(const char *string, void *arg) {

    if (string == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    printstr(&(PrintTokens){1, " >> ", NULL, string, COLOR_SEP(RED)}, (WindowManager*)arg);

    LOG(DEBUG, "-> extracted message \"%s\"", string);

    wrefresh(get_window(((WindowManager*)arg)->chatwin));
    wrefresh(get_window(((WindowManager*)arg)->inputwin));
}

void display_settings(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    printstr(&(PrintTokens){1, NULL, NULL, NULL, 0}, windowManager);
    printstr(&(PrintTokens){1, " ** ", NULL, "Assigned settings: ", COLOR_SEP(CYAN)}, windowManager);

    char propertyValue[MAX_CHARS + 1] = {'\0'};

    for (int i = 0; i < get_settings_capacity(); i++) {

        if (is_option_registered(i)) {

            memset(propertyValue, '\0', ARR_SIZE(propertyValue));
            propertyValue[0] = ' ';

            if (get_option_data_type(i) == INT_TYPE) {

                char valueStr[MAX_CHARS + 1] = {'\0'};

                int value = get_int_option_value(i);
                uint_to_str(valueStr, ARR_SIZE(valueStr), value);

                strcat(propertyValue, valueStr); 
            }
            else if (get_option_data_type(i) == CHAR_TYPE) {
                strcat(propertyValue, get_char_option_value(i));
            }

            printstr(&(PrintTokens){1, SPACE, get_option_label(i), propertyValue, STYLE_CNT(DIM)}, windowManager);
        }
    }
    wrefresh(get_window(windowManager->chatwin));
    wrefresh(get_window(windowManager->inputwin));
}

void display_time(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char timestamp[HM_TIME_LENGTH] = {'\0'};
    get_datetime(get_format_function(HM_TIME), timestamp, HM_TIME_LENGTH);

    draw_border(get_window(windowManager->statuswin), 0, 0, STATUS_START_POS);

    wattron(get_window(windowManager->statuswin), COLOR_PAIR(CYAN_REV));
    mvwaddstr(get_window(windowManager->statuswin), 0, 1, timestamp);
    wattroff(get_window(windowManager->statuswin), COLOR_PAIR(CYAN_REV));
    
    wrefresh(get_window(windowManager->statuswin));
    wrefresh(get_window(windowManager->inputwin));
}

void display_status(WindowManager *windowManager, const char *status, ...) {

    if (windowManager == NULL || status == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    // draw_border(get_window(windowManager->statuswin), 0, 0);
    draw_border(get_window(windowManager->statuswin), 0, STATUS_START_POS, 0);

    if (strchr(status, '%') == NULL) {

        wattron(get_window(windowManager->statuswin), COLOR_PAIR(CYAN_REV));
        mvwaddstr(get_window(windowManager->statuswin), 0, STATUS_START_POS, status);
        wattroff(get_window(windowManager->statuswin), COLOR_PAIR(CYAN_REV));
    }
    else {
        char statusWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, status);
        vsnprintf(statusWithArgs, MAX_CHARS, status, arglist);
        va_end(arglist);

        wattron(get_window(windowManager->statuswin), COLOR_PAIR(CYAN_REV));
        mvwaddstr(get_window(windowManager->statuswin), 0, 1, statusWithArgs);
        wattroff(get_window(windowManager->statuswin), COLOR_PAIR(CYAN_REV));
    }

    wrefresh(get_window(windowManager->statuswin));
    wrefresh(get_window(windowManager->inputwin));
}

STATIC void display_list_item(const char *string, void *arg) {

    printstr(&(PrintTokens){1, SPACE_2, NULL, string, 0}, (WindowManager*)arg);

}

void resize_ui(WindowManager *windowManager, int useColors) {

    const int MIN_ROWS = 4;

    /* temporarily exit ncurses mode and refresh to 
        reinitialize the terminal with the new size.
        after resizing, clean the window structures 
        and reinitialize the keypad */
    endwin();
    refresh();
    wclear(get_window(windowManager->chatwin));
    wclear(get_window(windowManager->statuswin));
    wclear(get_window(windowManager->inputwin));
    keypad(get_window(windowManager->inputwin), TRUE);

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

        int rowsChatWin = rows < MIN_ROWS ? 1 : rows - 3;

        wresize(get_window(windowManager->chatwin), rowsChatWin, cols);
        mvwin(get_window(windowManager->chatwin), 1, 0);
    }
    if (rows > 2) {

        int y = rows > 3 ? rows - 2 : rows - 1;

        wresize(get_window(windowManager->statuswin), 1, cols);
        mvwin(get_window(windowManager->statuswin), y, 0);
    }
    if (rows > 3) {

        wresize(get_window(windowManager->inputwin), 1, cols);
        mvwin(get_window(windowManager->inputwin), rows - 1, 0);
    }

    /* redraw window borders, reload the content from 
        the scrollback and display the last command */
    create_window_borders(windowManager, useColors);
    restore_from_scrollback(get_scrollback(windowManager->chatwin));

    mvwaddstr(get_window(windowManager->inputwin), 0, 0, PROMPT);
    display_current_command(get_line_editor(windowManager->inputwin));

    wrefresh(get_window(windowManager->inputwin));

}

UIWindow * get_titlewin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->titlewin;
}

UIWindow * get_chatwin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->chatwin;
}

UIWindow * get_statuswin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->statuswin;
}

UIWindow * get_inputwin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return windowManager->inputwin;
}

PrintFunc get_print_function(void) {
    
    return printFunc;
}

void set_print_function(PrintFunc pf) {

    printFunc = pf;
}
