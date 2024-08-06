// wide characters support
#define _XOPEN_SOURCE_EXTENDED 1

#include "display.h"
#include "input_handler.h"
#include "tcpclient.h"
#include "main.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <errno.h>

#include <wchar.h>

#define MAX_CHARS 256
#define TIMESTAMP_LENGTH 6
#define SCROLLBACK_MULTIPLIER 5

static void get_time(char *timestamp);

static WINDOW *mainwin = NULL;
static WINDOW *chatwin = NULL;
static WINDOW *statuswin = NULL;
static WINDOW *inputwin = NULL;

static Scrollback *scrollback = NULL;
static LineEditor *lnEditor = NULL;

WINDOW* get_mainwin(void) {
    return mainwin;
}

WINDOW* get_chatwin(void) {
    return chatwin;
}

WINDOW* get_statuswin(void) {
    return statuswin;
}

WINDOW* get_inputwin(void) {
    return inputwin;
}

Scrollback* get_scrollback(void) {
    return scrollback;
}

LineEditor* get_line_editor(void) {
    return lnEditor;
}

void init_curses(void) {

    // Set the program's locale to default environment 
    setlocale(LC_ALL, "");

    /* initialize curses data structures, 
    disable line buffering and disable
     character echo */
    initscr(); 
    cbreak();
    noecho(); 
}

void init_windows(void) {

    /*  window height:  
    mainwin - total height - 1 (inputwin)
    chatwin - mainwin height - 2 (borders)
    inputwin - 1 (single line) */
    mainwin = subwin(stdscr, get_wheight(stdscr)-1, get_wwidth(stdscr), 0, 0);
    chatwin = subwin(mainwin, get_wheight(mainwin)-2, get_wwidth(stdscr), 1, 0); 
    statuswin = subwin(mainwin, 1, get_wwidth(stdscr), get_wheight(mainwin)-1, 0);  
    inputwin = subwin(stdscr, 1, get_wwidth(stdscr), get_wheight(stdscr)-1, 0);

    // capture special keys
    keypad(inputwin, TRUE);
     // enable scrolling
    scrollok(chatwin, TRUE);     
    // optimize scrolling with escape sequences
    idlok(chatwin, TRUE);
  
}

void delete_windows(void) {

    // if (stdscr) {
    //     endwin();
    // }
    if (inputwin != NULL) { 
        delwin(inputwin);
    }
    if (chatwin != NULL) {
        delwin(chatwin);
    }
    if (statuswin != NULL) {
        delwin(statuswin);
    }
    if (mainwin != NULL) {
        delwin(mainwin);
    }
}

// create scrollback buffer and line editor
void create_buffers(void) {

    scrollback = create_scrollback(chatwin, get_wheight(chatwin) * SCROLLBACK_MULTIPLIER);
    lnEditor = create_line_editor(inputwin);
}

void delete_buffers(void) {

    delete_line_editor(lnEditor);
    delete_scrollback(scrollback);
}

void create_layout(void) {

    init_colors();   

    draw_window_borders();

    // print welcome messages
    printmsg(PRINT_STD, " ## ", NULL, "Buzz v0.1", COLOR_SEP(MAGENTA));
    printmsg(PRINT_TS, NULL, NULL, NULL, 0);
    printmsg(PRINT_STD, " ## ", NULL, "Type /help for a list of available commands.", COLOR_SEP(MAGENTA));

    wrefresh(mainwin);

    // set prompt to input window
    mvwaddstr(inputwin, 0, 0, PROMPT);
    wrefresh(inputwin);
}


// define color pairs
void init_colors(void) {

    if (has_colors()) {

        start_color();

        init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(RED, COLOR_RED, COLOR_BLACK);
        init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(CYAN_REV, COLOR_BLACK, COLOR_CYAN);
    }
    else {
        failed("No support for colors.");
    }
}

/* print formatted message (opt is a bitfield of items:
 timestamp, separator, origin and message,
 attr is a bitfield of attributes: color for 
 each item and style for message) */
void printmsg(uint8_t opt, const char *sep, const char *org, const char *msg, uint32_t attr) {

    char timestamp[TIMESTAMP_LENGTH];
    get_time(timestamp);

    int charCount = strlen(timestamp);

    charCount += (sep ? strlen(sep) : 0) +
                (org ? strlen(org) : 0) +
                (msg ? strlen(msg) : 0);

    if (charCount > MAX_CHARS) {
        return;
    }

    cchar_t line[MAX_CHARS+1] = {0};
    cchar_t *linePtr = line;

    if (opt & PRINT_TS) {
        linePtr += convert_to_cchar(linePtr, timestamp, 0);
    }
    if (opt & PRINT_SEP && sep) {
        linePtr += convert_to_cchar(linePtr, sep, (attr & 0xF));
    }
    if (opt & PRINT_ORG && org) {
        linePtr += convert_to_cchar(linePtr, org, (attr & 0xF0));
    }
    if (opt & PRINT_MSG && msg) {
        linePtr += convert_to_cchar(linePtr, msg, (attr & 0xFFFFFF00));
    }
    // linePtr += convert_to_cchar(linePtr, "\n", 0);

    add_to_scrollback(scrollback, line, linePtr - line);
    print_from_scrollback(scrollback, -1, scrollback->head);

}


// convert whchar_t to cchar
int convert_to_cchar(cchar_t *line, const char *string, uint32_t attr) {

    wchar_t buffer[MAX_CHARS+1] = {L'\0'};

    // get style from first 20 bits
    attr_t style = attr & 0xFFFFF000;

    int i = 0, color = 0;
    
    // get color pair from last 12 bits (sep, org, msg)
    while (!(color = attr & 0xF) && i++ < 3) {
        attr >>= 4;
    }

    if (mbstowcs(buffer, string, MAX_CHARS) == (size_t) -1) {
        failed("Error converting char to wchar.");
    }

    i = 0;
    while (buffer[i]) {
        setcchar(line++, &buffer[i++], style, color, NULL);
    }

    return i;
}


// get current time in hh:mm format
static void get_time(char *timestamp) {

    time_t timeVal;
    struct tm *timeInfo;

    timeVal = time(NULL);
    timeInfo = localtime(&timeVal);

    strftime(timestamp, TIMESTAMP_LENGTH, "%H:%M", timeInfo);
}

// display a list of available commands
void print_commands(const CommandInfo *cmdList) {

    if (cmdList == NULL) {
        failed("Invalid command list");
    }

    printmsg(PRINT_TS, NULL, NULL, NULL, 0);
    printmsg(PRINT_STD, " ** ", NULL, "Commands:", COLOR_SEP(CYAN) | A_STANDOUT);

    for (int i = 1; i < CMD_COUNT; i++) {
        printmsg(PRINT_STD, SPACE, NULL, cmdList[i].label, 0);
    }

    printmsg(PRINT_STD, SPACE, NULL, "For details type /help <command name>.", 0);

    wrefresh(scrollback->window);
    wrefresh(inputwin);
}


// display help on how to use a command
void print_usage(CommandInfo cmd) {

    printmsg(PRINT_TS, NULL, NULL, NULL, 0);
    printmsg(PRINT_STD, " ** ", NULL, cmd.label, COLOR_SEP(MAGENTA) | A_STANDOUT);
    printmsg(PRINT_STD, SPACE, NULL, cmd.usage, 0);

    wrefresh(scrollback->window);
    wrefresh(inputwin);
}

// display info message
void print_info(const char *info, ...) {
    
    if (strchr(info, '%') == NULL) {

        printmsg(PRINT_STD, " ** ", NULL, info, COLOR_SEP(BLUE) | A_BOLD);
    }
    else {
        char infoWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, info);
        vsnprintf(infoWithArgs, MAX_CHARS, info, arglist);
        va_end(arglist);
        printmsg(PRINT_STD, " ** ", NULL, infoWithArgs, COLOR_SEP(BLUE) | A_BOLD);
    }

    wrefresh(scrollback->window);
    wrefresh(inputwin);
}

// display status info (status window)
void print_status(const char *status, ...) {

    if (strchr(status, '%') == NULL) {

        wattron(statuswin, COLOR_PAIR(CYAN_REV));
        mvwaddstr(statuswin, 0, 1, status);
        wattroff(statuswin, COLOR_PAIR(CYAN_REV));
    }
    else {
        char statusWithArgs[MAX_CHARS] = {'\0'}; 
       
        va_list arglist;
        va_start(arglist, status);
        vsnprintf(statusWithArgs, MAX_CHARS, status, arglist);
        va_end(arglist);
        wattron(statuswin, COLOR_PAIR(CYAN_REV));
        mvwaddstr(statuswin, 0, 1, statusWithArgs);
        wattroff(statuswin, COLOR_PAIR(CYAN_REV));
    }

    wrefresh(statuswin);
    wrefresh(inputwin);
}

// adjust displayed information for a new window size
void handle_resize(void) {

    // clear();
    endwin();
    refresh();

    mvwin(mainwin, 0, 0);
    wresize(mainwin, get_wheight(stdscr)-1, get_wwidth(stdscr));

    mvwin(chatwin, 1, 0);
    wresize(chatwin, get_wheight(mainwin)-2, get_wwidth(stdscr));
 
    mvwin(statuswin, get_wheight(mainwin)-1, 0);
    mvwin(inputwin, get_wheight(stdscr)-1, 0);
    wclear(mainwin);
    wrefresh(mainwin);
    // delete_windows();
    // init_windows();
    // scrollback->window = chatwin;
    // lnEditor->window = inputwin;
    draw_window_borders();
    restore_from_scrollback(scrollback);
}


// draw top and bottom border with title
void draw_window_borders(void) {
    
    cchar_t block;
    setcchar(&block, BLOCK_CHAR, 0, CYAN, NULL);

    int cols = get_wwidth(mainwin);
    int rows = get_wheight(mainwin);

    mvwhline_set(mainwin, 0, 0, &block, cols); 
    mvwhline_set(mainwin, rows - 1, 0, &block, cols);

    // set window title
    wattron(mainwin, COLOR_PAIR(CYAN_REV));
    mvwaddstr(mainwin, 0, 1, "Buzz");
    wattroff(mainwin, COLOR_PAIR(CYAN_REV));
}


