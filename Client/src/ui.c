#define _XOPEN_SOURCE_EXTENDED 1
#include "ui.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncursesw/curses.h>

void mvwhline_set_color(WINDOW*, int, int, const cchar_t*, int, int);
void mvwprintw_color(WINDOW*, int, int, const char *, int);
void mvwprintw_time_color(WINDOW *, int *, int, const char *, const char *, const char *, int, int, int);
void apply_color_to_window(WINDOW *, int, int, int (*)(WINDOW *, int, int, const void *, int), const void *, int, int);

void get_time(char *, int);
void draw_borders(WINDOW *, int, const wchar_t *, int);


void initialize_colors(void) {
    start_color();

    init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);       // color pairs (foreground, background)
    init_pair(CYAN_BLACK, COLOR_CYAN, COLOR_BLACK);
    init_pair(BLACK_CYAN, COLOR_BLACK, COLOR_CYAN);
    init_pair(BLUE_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(MAGENTA_BLACK, COLOR_MAGENTA, COLOR_BLACK);
}

void setup_windows(WINDOW *topWin, WINDOW *bottomWin, int *currentLine) {

    draw_borders(topWin, 0, BLOCK_CHAR, SET_COLOR(CYAN_BLACK));
    mvwprintw_color(topWin, 0, 0, " Chat", SET_COLOR(BLACK_CYAN));

    mvwprintw_time_color(topWin, currentLine, 0, "", " ## ", "Chat v 0.1", 0, SET_COLOR(MAGENTA_BLACK), 0);
    mvwprintw_time_color(topWin, currentLine, 0, "", NULL, NULL, 0, SET_COLOR(MAGENTA_BLACK), 0);
    mvwprintw_time_color(topWin, currentLine, 0, "", " ## ", "Type /help for a list of available commands.", 0, SET_COLOR(MAGENTA_BLACK), 0);

    mvwprintw(bottomWin, 0, 0, PROMPT);

    wrefresh(topWin);
    wrefresh(bottomWin);
}

void mvwhline_set_color(WINDOW *win, int y, int x, const cchar_t *wch, int n, int color) {
    apply_color_to_window(win, y, x, (int (*)(WINDOW *, int, int, const void *, int))&mvwhline_set, wch, n, color);
}

void mvwprintw_color(WINDOW *win, int y, int x, const char *fmt, int color) {
    apply_color_to_window(win, y, x, (int (*)(WINDOW *, int, int, const void *, int))&mvwprintw, fmt, strlen(fmt), color);
}

void apply_color_to_window(WINDOW *win, int y, int x, int (*func)(WINDOW *, int, int, const void *, int), const void *arg, int n, int color) {
    wattron(win, color);
    func(win, y, x, arg, n);
    wattroff(win, color);
}

void mvwprintw_time_color(WINDOW *win, int *y, int x, const char *time, const char *separator, const char *string, int timeColor, int separatorColor, int stringColor) {

    int len = 0;

    if (time != NULL) {
        char timeBuffer[] = "hh:mm";

        get_time(timeBuffer, sizeof(timeBuffer));
        timeColor = (timeColor == 0) ? WHITE_BLACK : timeColor;

        mvwprintw_color(win, *y, x, timeBuffer, timeColor);
        len += strlen(timeBuffer);
    }

    if (separator != NULL) {
        separatorColor = (separatorColor == 0) ? WHITE_BLACK : separatorColor;

        mvwprintw_color(win, *y, x+len, separator, separatorColor);
        len += strlen(separator);
    }

    if (string != NULL) {
        stringColor = (stringColor == 0) ? WHITE_BLACK : stringColor;

        mvwprintw_color(win, *y, x+len, string, stringColor);
    }
    
    ++*y;
}

void get_time(char *timeBuffer, int n) {

    time_t rawtime;
    struct tm *time_s;

    time(&rawtime);
    time_s = localtime(&rawtime);

    strftime(timeBuffer, n,"%H:%M", time_s);

}

void shift_chars(char *inputBuffer, int x, int *charsWritten) {

    for (int i = x; i <= *charsWritten + 1; i++) {
        
        inputBuffer[i-PROMPT_SIZE] = inputBuffer[i-PROMPT_SIZE+1];
    }

    inputBuffer[*charsWritten-PROMPT_SIZE+1] = '\0';
    --*charsWritten;
}

void add_space(WINDOW *win, char *inputBuffer, int y, int x, int *charsWritten) {

    for (int i = *charsWritten + 1; i >= x; i--) {

        char key = mvwinch(win, y, i);
        mvwaddch(win, y, i+1, key);
        inputBuffer[i-PROMPT_SIZE+1] = inputBuffer[i-PROMPT_SIZE];
    }

    mvwaddch(win, y, x, ' ');

    inputBuffer[x-PROMPT_SIZE] = ' ';
    ++*charsWritten;
    wmove(win, y, x + 1);

}

CommandEnum get_user_command(char *inputBuffer, Command *commandLookup, int n) {

    for (int i = 0; i < n; i++) {
        if (strcmp(inputBuffer, commandLookup[i].commandString) == 0)
            return commandLookup[i].commandEnum;
    }
    return INVALID_COMMAND;
}

void display_help(WINDOW *win, int *y, Command *commandLookup, int n) {

    mvwprintw_time_color(win, y, 0, "", NULL, NULL, 0, 0, 0);
    mvwprintw_time_color(win, y, 0, "", " <> ", "Chat commands (precede with '/'):", 0, SET_COLOR(BLUE_BLACK), 0);

    for (int i = 0; i < n - 1; i++) {

        mvwprintw_time_color(win, y, 0, "", " ", &commandLookup[i].commandString[1], 0, 0, 0);
    }
    wrefresh(win);
}

void draw_borders(WINDOW *win, int x, const wchar_t *wch, int color) {
    
    cchar_t block;
    setcchar(&block, wch, 0, CYAN_BLACK, NULL);
    mvwhline_set_color(win, 0, x, &block, COLS-x, color);
    mvwhline_set_color(win, LINES-2, x, &block, COLS-x, color);
}

void handle_resize(WINDOW *topWin, int rows, int cols) {

    draw_borders(topWin, x, BLOCK_CHAR, SET_COLOR(CYAN_BLACK));
    
}
