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

#define MAX_MSG_TOKENS 5


/* defines conversion values between client
    and ncurses attribute enums */
static const unsigned ATTR_CONVERT[] = {
    A_NORMAL,
    A_BOLD,
    A_STANDOUT,
    A_DIM,
    A_ITALIC
};

#ifndef TEST

struct WindowManager {
    WINDOW *stdscr;
    WINDOW *titlewin;
    WINDOW *chatwin;
    WINDOW *statuswin;
    WINDOW *inputwin;
};

struct PrintTokens {
    int useTimestamp;
    const char *separator;
    const char *origin;
    const char *message;
};

#endif

STATIC int get_message_length(PrintTokens *printTokens);
STATIC void draw_window_borders(WindowManager *windowManager, int useColor);
STATIC void display_string_list(Scrollback *scrollback, const char **stringList, int count, const char *title);

WindowManager * create_windows(void) {

    WindowManager *windowManager = (WindowManager*) malloc(sizeof(WindowManager));
    if (windowManager == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    // set client locale to that of OS environment
    setlocale(LC_ALL, "");

    // initialize ncurses data structures
    windowManager->stdscr = initscr(); 

    /*  the entire terminal screen is divided into several
        sections (windows): 
        1. the entire terminal window managed by ncurses is stdscr, 
        2. the top line of stdscr is the "title window" which displays 
            the top border and the title,
        3. below the "title window" is the "chat window" 
            which extends until the last two lines of the stdscr.
            it displays commands, responses and messages,
        4. below the the "chat window" is the "status window" which
            displays the bottom border and status messages,
        5. the bottom line of stdscr is the "input window" which 
            displays user input */

    int rows = get_wheight(windowManager->stdscr);
    int cols = get_wwidth(windowManager->stdscr);

    windowManager->titlewin = newwin(1, cols, 0, 0);
    windowManager->chatwin = newwin(rows - 3, cols, 1, 0); 
    windowManager->statuswin = newwin(1, cols, rows - 2, 0);  
    windowManager->inputwin = newwin(1, cols, rows - 1, 0);

    return windowManager;
}

void delete_windows(WindowManager *windowManager) {

    if (windowManager != NULL) { 

        endwin();

        if (windowManager->inputwin != NULL) { 
            delwin(windowManager->inputwin);
        }
        if (windowManager->statuswin != NULL) {
            delwin(windowManager->statuswin);
        }
        if (windowManager->chatwin != NULL) {
            delwin(windowManager->chatwin);
        }
        if (windowManager->titlewin != NULL) {
            delwin(windowManager->titlewin);
        }
    }
    free(windowManager);
}

PrintTokens * create_print_tokens(int useTimestamp, const char *separator, const char *origin, const char *message) {

    PrintTokens *printTokens = (PrintTokens *) malloc(sizeof(PrintTokens));

    printTokens->useTimestamp = useTimestamp;
    printTokens->separator = separator;
    printTokens->origin = origin;
    printTokens->message = message;

    return printTokens;
}

void delete_print_tokens(PrintTokens *printTokens) {

    free(printTokens);
}

void set_windows_options(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    // disables line buffering and character echo
    cbreak();
    noecho(); 

    // captures special keys
    keypad(windowManager->inputwin, TRUE);
    // enables scrolling
    scrollok(windowManager->chatwin, TRUE);     
    // optimizes scrolling with escape sequences
    idlok(windowManager->chatwin, TRUE);

}

void init_colors(int useColor) {

    if (useColor && has_colors()) {

        start_color();

        init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(RED, COLOR_RED, COLOR_BLACK);
        init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(CYAN_REV, COLOR_BLACK, COLOR_CYAN);
    }

}

void create_layout(WindowManager *windowManager, Scrollback *scrollback, int useColor) {  

    if (windowManager == NULL || scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    draw_window_borders(windowManager, useColor);

    // displays standard application info
    printmsg(scrollback, &(PrintTokens){1, " ## ", NULL, "Buzz v1.0"}, COLOR_SEP(MAGENTA) | ATTR_MSG(BOLD));

    // displays active settings
    display_settings(scrollback);

    printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(PrintTokens){1, " ** ", NULL, "Type /help for a list of available commands."}, COLOR_SEP(CYAN));

    wrefresh(windowManager->chatwin);

    // displays input prompt
    mvwaddstr(windowManager->inputwin, 0, 0, PROMPT);
    wrefresh(windowManager->inputwin);
}

/* creates the main window borders. the top border 
    includes the app title and the bottom border
    represents the status window */
STATIC void draw_window_borders(WindowManager *windowManager, int useColor) {
    
    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    mvwhline_set(windowManager->titlewin, 0, 0, &block, get_wwidth(windowManager->titlewin));

    int attribute = useColor ? COLOR_PAIR(CYAN_REV) : A_REVERSE;

    wattron(windowManager->titlewin, attribute);
    mvwaddstr(windowManager->titlewin, 0, 1, "Buzz - IRC client");
    wattroff(windowManager->titlewin, attribute); 

    wrefresh(windowManager->titlewin);

    mvwhline_set(windowManager->statuswin, 0, 0, &block, get_wwidth(windowManager->statuswin));
    wrefresh(windowManager->statuswin);

}


void printmsg(Scrollback *scrollback, PrintTokens *printTokens, uint32_t attributes) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (get_message_length(printTokens) > MAX_CHARS) {
        FAILED("Max message length exceeded.", NO_ERRCODE);
    }

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = NULL;

    const char *tokens[MAX_MSG_TOKENS] = {NULL};
    int tkCount = 1;

    if (printTokens->message != NULL) {

        char *msgCopy = strdup(printTokens->message);
        tkCount = split_input_string(msgCopy, tokens, MAX_MSG_TOKENS, '\n');
    }
    int i = 0;

    /* the message is divided into several tokens. each 
        token is converted to cchar_t representation 
        with applied formatting */
    while (tkCount--) {

        bufferPtr = buffer;

        if (printTokens->useTimestamp) {

            char useTimestamp[HM_TIME_LENGTH] = {'\0'};
            get_datetime(get_format_function(HM_TIME), useTimestamp, HM_TIME_LENGTH);
            
            bufferPtr += string_to_complex_string(useTimestamp, bufferPtr, strlen(useTimestamp), 0);
        }
        if (printTokens->separator != NULL) {

            bufferPtr += string_to_complex_string(printTokens->separator, bufferPtr, strlen(printTokens->separator), (attributes & 0x00F00F));
        }
        if (printTokens->origin != NULL) {

            bufferPtr += string_to_complex_string(printTokens->origin, bufferPtr, strlen(printTokens->origin), (attributes & 0x0F00F0));
        }
        if (printTokens->message != NULL) {

            bufferPtr += string_to_complex_string(tokens[i++], bufferPtr, strlen(printTokens->message), (attributes & 0xF00F00));
        }

        /* message is stored in the scrollback buffer.
            after saving the maessage, it is displayed 
            on the screen */
        add_to_scrollback(scrollback, buffer, bufferPtr - buffer);
        print_from_scrollback(scrollback, -1, get_scrollback_head(scrollback));
    }
}

// calculates total message length
STATIC int get_message_length(PrintTokens *printTokens) {

    if (printTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int msgLen = 0;

    if (printTokens->useTimestamp) {
        msgLen += HM_TIME_LENGTH - 1;
    }
    if (printTokens->separator) {
        msgLen += strlen(printTokens->separator);
    }
    if (printTokens->origin) {
        msgLen += strlen(printTokens->origin);
    }
    if (printTokens->message) {
        msgLen += strlen(printTokens->message);
    }

    return msgLen;
}

int string_to_complex_string(const char *string, cchar_t *buffer, int len, uint32_t attributes) {

    if (string == NULL || buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    if (len > MAX_CHARS) {
        FAILED("String is too long for conversion.", NO_ERRCODE);
    }

    /* the color is extracted from the last 12 bits
        of the bit field and the style from the next
        12 bits. each 4 bits represent the style or 
        the color of a token */
    int color = (attributes & 0x00000FFF);
    attr_t style = (attributes & 0x00FFF000) >> 12;

    int i = 0;
    
    while (!(color &= 0xF) && ++i < 3) {
        color >>= 4;
        style >>= 4;
    }

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};
    int charsConverted = 0;

    /* converts chars to wide chars (wchar_t) */
    if ((charsConverted = mbstowcs(wstring, string, MAX_CHARS)) == -1) {
        FAILED("Error converting char to wchar", NO_ERRCODE);
    }

    wchar_t temp[2];
    i = 0;

    /* converts wide chars to complex chars (cchar_t) */
    while (i < charsConverted && wstring[i] != L'\0') {

        temp[0] = wstring[i];
        temp[1] = L'\0';  

        if (setcchar(buffer++, temp, ATTR_CONVERT[(Attributes)style], color, NULL) != OK) {
            FAILED("Error converting wchar to cchar", NO_ERRCODE);
        }
        i++;
    }

    return i;
}

int count_complex_chars(cchar_t *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0;

    while (string[i].chars[0] != L'\0') {

        i++;
    }

    return i;
}

void display_commands(Scrollback *scrollback, const Command *commands, int count) {

    if (scrollback == NULL || commands == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(PrintTokens){1, " ** ", NULL, "Commands:"}, COLOR_SEP(CYAN));

    int size = get_command_size();
    char *cmd = (char*) commands;

    for (int i = 1; i < count - 1; i++) {

        printmsg(scrollback, &(PrintTokens){1, SPACE, NULL, get_command_label((Command*)(cmd +  i * size))}, ATTR_MSG(DIM));
    }

    printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(PrintTokens){1, SPACE, NULL, "Type /help <command name> for usage details."}, 0);

    wrefresh(get_scrollback_window(scrollback));

}

void display_usage(Scrollback *scrollback, const Command *command) {

    printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(PrintTokens){1, " ** ", "Command ", get_command_label(command)}, COLOR_SEP(CYAN) | ATTR_MSG(DIM));

    printmsg(scrollback, &(PrintTokens){1, SPACE, "Syntax:", NULL}, 0);
    printmsg(scrollback, &(PrintTokens){1, SPACE_2, NULL, get_command_syntax(command)}, 0);

    display_string_list(scrollback, get_command_description(command), MAX_TOKENS, "Description:");
    display_string_list(scrollback, get_command_examples(command), MAX_TOKENS, "Example(s):");

    wrefresh(get_scrollback_window(scrollback));
}

void display_response(Scrollback *scrollback, const char *response, ...) {

    if (response == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    else if (strchr(response, '%') == NULL) {

        printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
        printmsg(scrollback, &(PrintTokens){1, " ** ", NULL, response}, COLOR_SEP(CYAN) | ATTR_MSG(BOLD));
    }
    else {
        char responseWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, response);
        vsnprintf(responseWithArgs, MAX_CHARS, response, arglist);
        va_end(arglist);

        printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
        printmsg(scrollback, &(PrintTokens){1, " ** ", NULL, responseWithArgs}, COLOR_SEP(CYAN) | ATTR_MSG(BOLD));

    }
    wrefresh(get_scrollback_window(scrollback));
}

void display_settings(Scrollback *scrollback) {

    printmsg(scrollback, &(PrintTokens){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(PrintTokens){1, " ** ", NULL, "Assigned settings: "}, COLOR_SEP(CYAN));

    char propertyValue[MAX_CHARS + 1];

    for (int i = 0; i < PROPERTY_TYPE_COUNT - 1; i++) {

        if (get_property_user((PropertyType) i) == CLIENT_PROPERTY && is_property_assigned((PropertyType) i)) {

            memset(propertyValue, '\0', sizeof(propertyValue));
            propertyValue[0] = ' ';

            strcat(propertyValue, get_property_value((PropertyType) i)); 

            printmsg(scrollback, &(PrintTokens){1, SPACE, property_type_to_string((PropertyType) i), propertyValue}, ATTR_MSG(DIM));
        }
    }
    wrefresh(get_scrollback_window(scrollback));
}

void display_status(WindowManager *windowManager, const char *status, ...) {

    if (status == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strchr(status, '%') == NULL) {

        wattron(windowManager->statuswin, COLOR_PAIR(CYAN_REV));
        mvwaddstr(windowManager->statuswin, 0, 1, status);
        wattroff(windowManager->statuswin, COLOR_PAIR(CYAN_REV));
    }
    else {
        char statusWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, status);
        vsnprintf(statusWithArgs, MAX_CHARS, status, arglist);
        va_end(arglist);

        wattron(windowManager->statuswin, COLOR_PAIR(CYAN_REV));
        mvwaddstr(windowManager->statuswin, 0, 1, statusWithArgs);
        wattroff(windowManager->statuswin, COLOR_PAIR(CYAN_REV));
    }

    wrefresh(windowManager->statuswin);
    wrefresh(windowManager->inputwin);
}


STATIC void display_string_list(Scrollback *scrollback, const char **stringList, int count, const char *title) {

    int i = 0;

    while (i < count && stringList[i] != NULL) {

        if (i == 0) {
            printmsg(scrollback, &(PrintTokens){1, SPACE, NULL, title}, 0);
        }
        printmsg(scrollback, &(PrintTokens){1, SPACE_2, NULL, stringList[i]}, 0);
        i++;
    }
}

void repaint_ui(WindowManager *windowManager, Scrollback *scrollback, LineEditor *lnEditor, int useColors) {

    const int MIN_ROWS = 4;

    /* temporarily exits ncurses mode and clears
        all windows to reinitialize terminal with
        the new size */
    endwin();
    refresh();
    wclear(windowManager->chatwin);
    wclear(windowManager->statuswin);
    wclear(windowManager->inputwin);
    keypad(windowManager->inputwin, TRUE);

    int rows = get_wheight(windowManager->stdscr);
    int cols = get_wwidth(windowManager->stdscr);

    /* hides cursor if the terminal is resized below
        required minimum to display the "input
        window" */
    if (rows < MIN_ROWS && curs_set(0)) {
        curs_set(0);
    }
    else if (rows >= MIN_ROWS && !curs_set(1)) {
        curs_set(1);
    }

    if (rows > 1) {

        int rowsChatWin = rows < MIN_ROWS ? 1 : rows - 3;

        wresize(windowManager->chatwin, rowsChatWin, cols);
        mvwin(windowManager->chatwin, 1, 0);
    }
    if (rows > 2) {

        int y = rows > 3 ? rows - 2 : rows - 1;

        wresize(windowManager->statuswin, 1, cols);
        mvwin(windowManager->statuswin, y, 0);
    }
    if (rows > 3) {

        wresize(windowManager->inputwin, 1, cols);
        mvwin(windowManager->inputwin, rows - 1, 0);
    }

    draw_window_borders(windowManager, useColors);
    restore_from_scrollback(scrollback);

    mvwaddstr(windowManager->inputwin, 0, 0, PROMPT);
    display_current_command(lnEditor);

    wrefresh(windowManager->inputwin);

}

WINDOW *get_chatwin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return windowManager->chatwin;
}

WINDOW *get_statuswin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return windowManager->statuswin;
}

WINDOW *get_inputwin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return windowManager->inputwin;
}
