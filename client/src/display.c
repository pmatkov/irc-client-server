// wide characters support
#define _XOPEN_SOURCE_EXTENDED 1

#ifdef TEST
#include "test_display.h"
#else
#include "display.h"
#endif

#include "line_editor.h"
#include "tcpclient.h"
#include "../../shared/src/time.h"
#include "../../shared/src/parser.h"
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

#define MAX_CHARS 512
#define MAX_TOKENS 5

#ifndef TEST

struct WindowManager {
    WINDOW *stdscr;
    WINDOW *mainwin;
    WINDOW *chatwin;
    WINDOW *statuswin;
    WINDOW *inputwin;
};

struct MessageParams{
    int timestamp;
    const char *separator;
    const char *origin;
    const char *message;
};

#endif

STATIC int get_message_length(MessageParams *msgParams);

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

    if (windowManager == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

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
void init_colors(Settings *settings) {

    if (settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char *useColors = get_property_value(settings, COLOR);

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

void create_layout(WindowManager *windowManager, Scrollback *scrollback, Settings *settings) {  

    if (windowManager == NULL || scrollback == NULL || settings == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    draw_window_borders(windowManager);

    // display app info
    printmsg(scrollback, (MessageParams){1, " ## ", NULL, "Buzz v1.0"}, COLOR_SEP(MAGENTA));
    printmsg(scrollback, (MessageParams){1, NULL, NULL, NULL}, 0);

    display_settings(scrollback, settings);
    printmsg(scrollback, (MessageParams){1, NULL, NULL, NULL}, 0);

    printmsg(scrollback, (MessageParams){1, " ## ", NULL, "Type /help for a list of available commands."}, COLOR_SEP(MAGENTA));

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
    mvwaddstr(windowManager->mainwin, 0, 1, "Buzz");
    wattroff(windowManager->mainwin, COLOR_PAIR(CYAN_REV));
}

/* print formatted message (opt is a bitfield of items:
 timestamp, separator, origin and message,
 attr is a bitfield of attributes: color for 
 each item and style for message) */

void printmsg(Scrollback *scrollback, MessageParams msgParams, uint32_t attributes) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (get_message_length(&msgParams) > MAX_CHARS) {
        FAILED("Max message length exceeded", NO_ERRCODE);
    }

    cchar_t buffer[MAX_CHARS + 1] = {0};
    cchar_t *bufferPtr = NULL;

    char *tokens[MAX_TOKENS] = {NULL};
    int tkCount = 1;

    if (msgParams.message != NULL) {

        char *msgCopy = strdup(msgParams.message);
        tkCount = split_input_string(msgCopy, tokens, MAX_TOKENS, '\n');
    }
    int i = 0;

    while (tkCount--) {

        bufferPtr = buffer;

        if (msgParams.timestamp) {

            char timestamp[TIME_LENGTH] = {'\0'};
            get_datetime(get_format_function(TIME), timestamp, TIME_LENGTH);
            
            bufferPtr += string_to_complex_string(timestamp, bufferPtr, strlen(timestamp), 0);
        }
        if (msgParams.separator != NULL) {

            bufferPtr += string_to_complex_string(msgParams.separator, bufferPtr, strlen(msgParams.separator), (attributes & 0xF));
        }
        if (msgParams.origin != NULL) {

            bufferPtr += string_to_complex_string(msgParams.origin, bufferPtr, strlen(msgParams.origin), (attributes & 0xF0));
        }
        if (msgParams.message != NULL) {

            bufferPtr += string_to_complex_string(tokens[i++], bufferPtr, strlen(msgParams.message), (attributes & 0xFFFFFF00));
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
        msgLen += TIME_LENGTH - 1;
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
    attr_t style = attr & 0xFFFFF000;

    int i = 0, color = 0;
    
    // get color pair from last 12 bits (sep, org, msg)
    while (!(color = attr & 0xF) && i++ < 3) {
        attr >>= 4;
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

        if (setcchar(buffer++, temp, style, color, NULL) != OK) {
            FAILED("Error setting cchar", NO_ERRCODE);
        }
        i++;
    }

    return i;
}

// display a list of available commands
void display_commands(Scrollback *scrollback) {

    if (scrollback == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    printmsg(scrollback, (MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, (MessageParams){1, " ** ", NULL, "Commands:"}, COLOR_SEP(CYAN) | A_STANDOUT);

    for (int i = 1; i < COMMAND_TYPE_COUNT - 1; i++) {

        printmsg(scrollback, (MessageParams){1, SPACE, NULL, get_command_label((CommandType)i)}, 0);
    }

    printmsg(scrollback, (MessageParams){1, SPACE, NULL, "For details type /help <command name>."}, 0);

    wrefresh(sb_get_window(scrollback));
    // wrefresh(inputwin);
}

// display help on how to use a command
void display_usage(Scrollback *scrollback, CommandType commandType, const char *usage) {

    printmsg(scrollback, (MessageParams){1, NULL, NULL, NULL}, 0);
    printmsg(scrollback, (MessageParams){1, " ** ", NULL, get_command_label(commandType)}, COLOR_SEP(CYAN) | A_STANDOUT);
    printmsg(scrollback, (MessageParams){1, SPACE, NULL, usage}, 0);

    wrefresh(sb_get_window(scrollback));
    // wrefresh(inputwin);
}

// display info message
void display_response(Scrollback *scrollback, const char *info, ...) {

    if (info == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    else if (strchr(info, '%') == NULL) {

        printmsg(scrollback, (MessageParams){1, NULL, NULL, NULL}, 0);
        printmsg(scrollback, (MessageParams){1, " ** ", NULL, info}, COLOR_SEP(CYAN) | A_BOLD);

    }
    else {
        char infoWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, info);
        vsnprintf(infoWithArgs, MAX_CHARS, info, arglist);
        va_end(arglist);

        printmsg(scrollback, (MessageParams){1, NULL, NULL, NULL}, 0);
        printmsg(scrollback, (MessageParams){1, " ** ", NULL, infoWithArgs}, COLOR_SEP(CYAN) | A_BOLD);

    }

    wrefresh(sb_get_window(scrollback));
    // wrefresh(inputwin);
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
void display_settings(Scrollback *scrollback, Settings *settings) {

    printmsg(scrollback, (MessageParams){1, " ** ", NULL, "Active settings: "}, COLOR_SEP(CYAN));

    char propertyValue[MAX_CHARS + 1];

    for (int i = 0; i < PROPERTY_TYPE_COUNT - 1; i++) {

        if (is_property_assigned(settings, (PropertyType)i)) {

            memset(propertyValue, '\0', MAX_CHARS + 1);
            propertyValue[0] = ' ';

            strcat(propertyValue, get_property_value(settings, (PropertyType)i)); 

            printmsg(scrollback, (MessageParams){1, SPACE, get_property_type_string((PropertyType)i), propertyValue}, 0);
        }
    }
    
    printmsg(scrollback, (MessageParams){1, SPACE, NULL, "Change nickname with /nick"}, 0);
    printmsg(scrollback, (MessageParams){1, SPACE, NULL, "Change username, hostname and real name with /user"}, 0);
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




