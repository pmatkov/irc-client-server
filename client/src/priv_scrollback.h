/* --INTERNAL HEADER--
    used for testing */

#ifndef SCROLLBACK_H
#define SCROLLBACK_H

#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

typedef struct {
    WINDOW *window;
    cchar_t **buffer;
    int tail;
    int head;
    int topLine;
    int bottomLine;
    int capacity;
    int count;
} Scrollback;

typedef void (*ScrollbackFunc)(Scrollback *scrollback);

typedef struct {
    int keyCode;
    ScrollbackFunc scrollbackFunc;
} ScrollbackCmd;

Scrollback * create_scrollback(WINDOW *window, int sizeMultiplier);
void delete_scrollback(Scrollback *scrollback); 

int is_scrollback_empty(Scrollback *scrollback);
int is_scrollback_full(Scrollback *scrollback);

void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length);
void print_from_scrollback(Scrollback *scrollback, int lineWnd, int lineSb);
void restore_from_scrollback(Scrollback *scrollback);

void scroll_line_up(Scrollback *scrollback);
void scroll_line_down(Scrollback *scrollback);
void scroll_page_up(Scrollback *scrollback);
void scroll_page_down(Scrollback *scrollback);

ScrollbackFunc get_scrollback_function(int index);
int get_sb_func_index(int keyCode);

int remap_ctrl_key(int ch);

WINDOW * get_scrollback_window(Scrollback *scrollback);
int get_scrollback_head(Scrollback *scrollback);

#ifdef TEST

int count_visible_lines(Scrollback *scrollback);
int count_remaining_top_lines(Scrollback *scrollback);
int count_remaining_bottom_lines(Scrollback *scrollback);

#endif

#endif
