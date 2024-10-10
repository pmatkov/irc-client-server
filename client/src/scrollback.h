#ifndef SCROLLBACK_H
#define SCROLLBACK_H

/* activates ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

/* a ScrollbackCmd represents a map of scrollback commands 
    and functions */
typedef struct ScrollbackCmd ScrollbackCmd;

/* a scrollback buffer stores visible text and 
    retains off-screen text for later retrieval 
    (depending on the scrollback size). the screen
    in this context means the "chat window". 
    
    the text can be scrolled in the following ways:
    - CTRL + UP ARROW scrolls one row up,
    - CTRL + DOWN ARROW scrolls one row down,
    - PG_UP scrolls one screen up,
    - PG_DOWN scrolls one screen down */
typedef struct Scrollback Scrollback;

typedef void (*ScrollbackFunction)(Scrollback *scrollback);

Scrollback * create_scrollback(WINDOW *window, int sizeMultiplier);
void delete_scrollback(Scrollback *scrollback); 

int is_scrollback_empty(Scrollback *scrollback);
int is_scrollback_full(Scrollback *scrollback);

/* saves a new line of text to the scrollback 
    buffer */
void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length);

/* displays a line of text from the scrollback 
    buffer.if lineWnd is -1, the line is displayed 
    at the current cursor position. otherwise,
    the line is displayed at the lineWnd position
    (effectively a window row) */
void print_from_scrollback(Scrollback *scrollback, int lineWnd, int lineSb);

/* when the terminal is resized, the visible
    part of the scrollback will be repainted
    to adjust the displayed text to the new window 
    size */
void restore_from_scrollback(Scrollback *scrollback);

/* below functions manipulate the visible part
    of the scrollback buffer */
void scroll_line_up(Scrollback *scrollback);
void scroll_line_down(Scrollback *scrollback);
void scroll_page_up(Scrollback *scrollback);
void scroll_page_down(Scrollback *scrollback);

ScrollbackFunction get_scrollback_function(int index);
int get_sb_func_index(int keyCode);

/* detects CTRL + arrow key combinations 
    which are used for scrolling */
int remap_ctrl_key(int ch);

WINDOW * get_scrollback_window(Scrollback *scrollback);
int get_scrollback_head(Scrollback *scrollback);

#endif
