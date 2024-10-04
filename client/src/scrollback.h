#ifndef SCROLLBACK_H
#define SCROLLBACK_H

#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

typedef struct ScrollbackCmd ScrollbackCmd;
typedef struct Scrollback Scrollback;

typedef void (*ScrollbackFunction)(Scrollback *scrollback);

Scrollback * create_scrollback(WINDOW *window, int sbMultiplier);
void delete_scrollback(Scrollback *scrollback); 

WINDOW * sb_get_window(Scrollback *scrollback);
int sb_get_head(Scrollback *scrollback);

int sb_is_empty(Scrollback *scrollback);
int sb_is_full(Scrollback *scrollback);
int get_preceding_ln_count(Scrollback *scrollback);

void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length);
void print_from_scrollback(Scrollback *scrollback, int lineWnd, int lineSb);
void restore_from_scrollback(Scrollback *scrollback);

void scroll_line_up(Scrollback *scrollback);
void scroll_line_down(Scrollback *scrollback);
void scroll_page_up(Scrollback *scrollback);
void scroll_page_down(Scrollback *scrollback);

ScrollbackFunction get_scrollback_function(int index);
int get_sb_func_index(int keyCode);

int remap_ctrl_key(int ch);

#endif
