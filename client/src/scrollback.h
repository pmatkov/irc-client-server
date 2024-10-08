#ifndef SCROLLBACK_H
#define SCROLLBACK_H

/* activates ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

/* ScrollbackCmd represents a map of scrollback commands 
    and functions */
typedef struct ScrollbackCmd ScrollbackCmd;

/* Scrollback is used to store on-screen text 
    and provide storage for some off-screen text 
    (depending on the scrollback size). the screen
    in this context means just the "chat window". 
    
    the text is scrolled like this:
    - CTRL + UP ARROW scrolls one row up,
    - CTRL + DOWN ARROW scrolls one row down,
    - PG_UP scrolls one screen up with,
    - PG_DOWN scrolls one screen down */
typedef struct Scrollback Scrollback;

typedef void (*ScrollbackFunction)(Scrollback *scrollback);

Scrollback * create_scrollback(WINDOW *window, int sizeMultiplier);
void delete_scrollback(Scrollback *scrollback); 

int is_scrollback_empty(Scrollback *scrollback);
int is_scrollback_full(Scrollback *scrollback);

/* counts the lines from the top of the scrollback
    until the last displayed line from the scrollback */
int get_preceding_line_count(Scrollback *scrollback);

/* saves a new line of text to the scrollback buffer */
void add_to_scrollback(Scrollback *scrollback, const cchar_t *string, int length);

/* displays a line from the scrollback on 
    screen. if lineWnd is -1, the line is 
    displayed at the current cursor position. 
    otherwise, the line is displayed at the 
    lineWnd position (in effect a row of the
    window) */
void print_from_scrollback(Scrollback *scrollback, int lineWnd, int lineSb);

/* used when the terminal is resized. 
    the scrollback is repainted according 
    to the new terminal size */
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
