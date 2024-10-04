#ifdef TEST
#include "priv_display.h"
#else
#include "display.h"
#endif

#include "../../shared/src/settings.h"
#include "../../shared/src/time_utils.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

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
    WINDOW *mainwin;
    WINDOW *chatwin;
    WINDOW *statuswin;
    WINDOW *inputwin;
};

struct MessageParams {
    int timestamp;
    const char *separator;
    const char *origin;
    const char *message;
};

#endif

STATIC int get_message_length(MessageParams *msgParams);
STATIC void display_string_list(Scrollback *scrollback, char **stringList, int size, const char *title);

WindowManager * create_windows(void) {

    WindowManager *windowManager = (WindowManager*) malloc(sizeof(WindowManager));
    if (windowManager == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    // Set the program's locale to default environment 
    setlocale(LC_ALL, "");

    windowManager->stdscr = initscr(); 

    /*  window height:  
    mainwin - total height - 1 (inputwin)
    chatwin - mainwin height - 2 (borders)
    inputwin - 1 (single line) */
    windowManager->mainwin = subwin(windowManager->stdscr, get_wheight(windowManager->stdscr)-1, get_wwidth(windowManager->stdscr), 0, 0);
    windowManager->chatwin = subwin(windowManager->mainwin, get_wheight(windowManager->mainwin)-2, get_wwidth(windowManager->stdscr), 1, 0); 
    windowManager->statuswin = subwin(windowManager->mainwin, 1, get_wwidth(windowManager->stdscr), get_wheight(windowManager->mainwin)-1, 0);  
    windowManager->inputwin = subwin(windowManager->stdscr, 1, get_wwidth(windowManager->stdscr), get_wheight(windowManager->stdscr)-1, 0);

    return windowManager;
}

void delete_windows(WindowManager *windowManager) {

    if (windowManager != NULL) { 

        if (windowManager->stdscr != NULL) {
            endwin();
        }
        if (windowManager->inputwin != NULL) { 
            delwin(windowManager->inputwin);
        }
        if (windowManager->chatwin != NULL) {
            delwin(windowManager->chatwin);
        }
        if (windowManager->statuswin != NULL) {
            delwin(windowManager->statuswin);
        }
        if (windowManager->mainwin != NULL) {
            delwin(windowManager->mainwin);
        }
    }

    free(windowManager);
}

WINDOW *get_chatwin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return windowManager->chatwin;
}

WINDOW *get_inputwin(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return windowManager->inputwin;
}

MessageParams * create_message_params(int timestamp, const char *separator, const char *origin, const char *message) {

    MessageParams *messageParams = (MessageParams *) malloc(sizeof(MessageParams));

    messageParams->timestamp = timestamp;
    messageParams->separator = separator;
    messageParams->origin = origin;
    messageParams->message = message;

    return messageParams;
}

void set_windows_options(WindowManager *windowManager) {

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    /* initialize curses data structures, 
    disable line buffering and disable
     character echo */
    cbreak();
    noecho(); 

    // capture special keys
    keypad(windowManager->inputwin, TRUE);
     // enable scrolling
    scrollok(windowManager->chatwin, TRUE);     
    // optimize scrolling with escape sequences
    idlok(windowManager->chatwin, TRUE);

}

// define color pairs
void init_colors(void) {

    const char *useColors = get_property_value(COLOR);

    if (str_to_uint(useColors) && has_colors()) {

        start_color();

        init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(RED, COLOR_RED, COLOR_BLACK);
        init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(CYAN_REV, COLOR_BLACK, COLOR_CYAN);
    }
}

void create_layout(WindowManager *windowManager, Scrollback *scrollback) {  

    if (windowManager == NULL || scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    draw_window_borders(windowManager);

    // display app info
    printmsg(scrollback, &(MessageParams){1, " ## ", NULL, "Buzz v1.0"}, COLOR_SEP(MAGENTA) | ATTR_MSG(BOLD));

    display_settings(scrollback);

    printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(MessageParams){1, " ** ", NULL, "Type /help for a list of available commands."}, COLOR_SEP(CYAN));

    wrefresh(windowManager->mainwin);

    // set prompt to input window
    mvwaddstr(windowManager->inputwin, 0, 0, PROMPT);
    wrefresh(windowManager->inputwin);
}

// draw top and bottom border with title
void draw_window_borders(WindowManager *windowManager) {
    
    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    int cols = get_wwidth(windowManager->mainwin);
    int rows = get_wheight(windowManager->mainwin);

    mvwhline_set(windowManager->mainwin, 0, 0, &block, cols); 
    mvwhline_set(windowManager->mainwin, rows - 1, 0, &block, cols);

    // set window title
    wattron(windowManager->mainwin, COLOR_PAIR(CYAN_REV));
    mvwaddstr(windowManager->mainwin, 0, 1, "Buzz - chat client");
    wattroff(windowManager->mainwin, COLOR_PAIR(CYAN_REV));
}

/* print formatted message (opt is a bitfield of items:
 timestamp, separator, origin and message,
 attr is a bitfield of attributes: color for 
 each item and style for message) */

void printmsg(Scrollback *scrollback, MessageParams *msgParams, uint32_t attributes) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (get_message_length(msgParams) > MAX_CHARS) {
        FAILED("Max message length exceeded", NO_ERRCODE);
    }

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = NULL;

    const char *tokens[MAX_MSG_TOKENS] = {NULL};
    int tkCount = 1;

    if (msgParams->message != NULL) {

        char *msgCopy = strdup(msgParams->message);
        tkCount = split_input_string(msgCopy, tokens, MAX_MSG_TOKENS, '\n');
    }
    int i = 0;

    while (tkCount--) {

        bufferPtr = buffer;

        if (msgParams->timestamp) {

            char timestamp[HM_TIME_LENGTH] = {'\0'};
            get_datetime(get_format_function(HM_TIME), timestamp, HM_TIME_LENGTH);
            
            bufferPtr += string_to_complex_string(timestamp, bufferPtr, strlen(timestamp), 0);
        }
        if (msgParams->separator != NULL) {

            bufferPtr += string_to_complex_string(msgParams->separator, bufferPtr, strlen(msgParams->separator), (attributes & 0x00F00F));
        }
        if (msgParams->origin != NULL) {

            bufferPtr += string_to_complex_string(msgParams->origin, bufferPtr, strlen(msgParams->origin), (attributes & 0x0F00F0));
        }
        if (msgParams->message != NULL) {

            bufferPtr += string_to_complex_string(tokens[i++], bufferPtr, strlen(msgParams->message), (attributes & 0xF00F00));
        }

        add_to_scrollback(scrollback, buffer, bufferPtr - buffer);
        print_from_scrollback(scrollback, -1, sb_get_head(scrollback));
    }
}

STATIC int get_message_length(MessageParams *msgParams) {

    if (msgParams == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int msgLen = 0;

    if (msgParams->timestamp) {
        msgLen += HM_TIME_LENGTH - 1;
    }
    if (msgParams->separator) {
        msgLen += strlen(msgParams->separator);
    }
    if (msgParams->origin) {
        msgLen += strlen(msgParams->origin);
    }
    if (msgParams->message) {
        msgLen += strlen(msgParams->message);
    }

    return msgLen;
}

// convert whchar_t to cchar
int string_to_complex_string(const char *string, cchar_t *buffer, int len, uint32_t attr) {

    if (string == NULL || buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    if (len > MAX_CHARS) {
        FAILED("Too long string for conversion", NO_ERRCODE);
    }

    // get style from first 20 bits
    // attr_t style = attr & 0xFFFFF000;
    attr_t style = (attr & 0x00FFF000) >> 12;
    int color = (attr & 0x00000FFF);

    int i = 0;
    
    // get color pair from last 12 bits (sep, org, msg)
    while (!(color &= 0xF) && ++i < 3) {
        color >>= 4;
        style >>= 4;
    }

    wchar_t wstring[MAX_CHARS + 1] = {L'\0'};
    int charsConverted = 0;

    if ((charsConverted = mbstowcs(wstring, string, MAX_CHARS)) == -1) {
        FAILED("Error converting char to wchar", NO_ERRCODE);
    }

    wchar_t temp[2];
    i = 0;

    while (i < charsConverted && wstring[i] != L'\0') {

        temp[0] = wstring[i];
        temp[1] = L'\0';  

        if (setcchar(buffer++, temp, ATTR_CONVERT[(Attributes)style], color, NULL) != OK) {
            FAILED("Error setting cchar", NO_ERRCODE);
        }
        i++;
    }

    return i;
}

// display a list of available commands
void display_commands(Scrollback *scrollback, const Command *commands, int count) {

    if (scrollback == NULL || commands == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(MessageParams){1, " ** ", NULL, "Commands:"}, COLOR_SEP(CYAN));

    int size = get_command_size();
    char *cmd = (char*) commands;

    for (int i = 1; i < count - 1; i++) {

        printmsg(scrollback, &(MessageParams){1, SPACE, NULL, get_command_label((Command*)(cmd +  i * size))}, 0);
    }

    printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(MessageParams){1, SPACE, NULL, "Type /help <command name> for usage details."}, 0);

    wrefresh(sb_get_window(scrollback));

}

// display help on how to use a command
void display_usage(Scrollback *scrollback, const Command *command) {

    printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(MessageParams){1, " ** ", "Command ", get_command_label(command)}, COLOR_SEP(CYAN) | ATTR_MSG(DIM));

    char *description[MAX_TOKENS] = {NULL};
    char *examples[MAX_TOKENS] = {NULL};

    get_command_description(command, description, MAX_TOKENS);
    get_command_examples(command, examples, MAX_TOKENS);

    printmsg(scrollback, &(MessageParams){1, SPACE, "Syntax:", NULL}, 0);
    printmsg(scrollback, &(MessageParams){1, SPACE_2, NULL, get_command_syntax(command)}, 0);

    display_string_list(scrollback, description, MAX_TOKENS, "Description:");
    display_string_list(scrollback, examples, MAX_TOKENS, "Example(s):");

    wrefresh(sb_get_window(scrollback));
}

// display client response
void display_response(Scrollback *scrollback, const char *response, ...) {

    if (response == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    else if (strchr(response, '%') == NULL) {

        printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
        printmsg(scrollback, &(MessageParams){1, " ** ", NULL, response}, COLOR_SEP(CYAN) | ATTR_MSG(BOLD));

    }
    else {
        char responseWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, response);
        vsnprintf(responseWithArgs, MAX_CHARS, response, arglist);
        va_end(arglist);

        printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
        printmsg(scrollback, &(MessageParams){1, " ** ", NULL, responseWithArgs}, COLOR_SEP(CYAN) | ATTR_MSG(BOLD));

    }
    wrefresh(sb_get_window(scrollback));
}

// display status info (status window)
void display_status(WindowManager *windowManager, const char *status, ...) {

    if (status == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    else if (strchr(status, '%') == NULL) {

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

// display settings
void display_settings(Scrollback *scrollback) {

    printmsg(scrollback, &(MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, &(MessageParams){1, " ** ", NULL, "Assigned settings: "}, COLOR_SEP(CYAN));

    char propertyValue[MAX_CHARS + 1];

    for (int i = 0; i < PROPERTY_TYPE_COUNT - 1; i++) {

        if (get_property_user((PropertyType) i) == CLIENT_PROPERTY && is_property_assigned((PropertyType) i)) {

            memset(propertyValue, '\0', sizeof(propertyValue));
            propertyValue[0] = ' ';

            strcat(propertyValue, get_property_value((PropertyType) i)); 

            printmsg(scrollback, &(MessageParams){1, SPACE, property_type_to_string((PropertyType) i), propertyValue}, ATTR_MSG(DIM));
        }
    }
}

// adjust display when resizing window
void handle_resize(WindowManager *windowManager, Scrollback *scrollback) {

    // clear();
    endwin();
    refresh();

    mvwin(windowManager->mainwin, 0, 0);
    wresize(windowManager->mainwin, get_wheight(windowManager->stdscr)-1, get_wwidth(windowManager->stdscr));

    mvwin(windowManager->chatwin, 1, 0);
    wresize(windowManager->chatwin, get_wheight(windowManager->mainwin)-2, get_wwidth(windowManager->stdscr));
 
    mvwin(windowManager->statuswin, get_wheight(windowManager->mainwin)-1, 0);
    mvwin(windowManager->inputwin, get_wheight(windowManager->stdscr)-1, 0);
    wclear(windowManager->mainwin);
    wrefresh(windowManager->mainwin);
    // delete_windows();
    // create_windows();
    // scrollback->window = chatwin;
    // lnEditor->window = inputwin;
    draw_window_borders(windowManager);
    restore_from_scrollback(scrollback);
}

STATIC void display_string_list(Scrollback *scrollback, char **stringList, int size, const char *title) {

    int i = 0;

    while (i < size && stringList[i] != NULL) {

        if (i == 0) {
            printmsg(scrollback, &(MessageParams){1, SPACE, NULL, title}, 0);
        }
        printmsg(scrollback, &(MessageParams){1, SPACE_2, NULL, stringList[i]}, 0);
        i++;
    }
}




