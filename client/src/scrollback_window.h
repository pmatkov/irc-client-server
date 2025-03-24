#ifndef SCROLLBACK_WINDOW_H
#define SCROLLBACK_WINDOW_H

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include "scroll_subject.h"
#include "base_window.h"

#include <ncursesw/curses.h>

/**
 *  interface for storing off-screen text with a predefined capacity.
 *  additional entries will overwrite the oldest entries in the buffer.
 *
*    The text can be scrolled with the following keys:
*    - CTRL + UP ARROW: Scrolls one row up
*    - CTRL + DOWN ARROW: Scrolls one row down
*    - PAGE UP: Scrolls one screen up
*    - PAGE DOWN: Scrolls one screen down
 */
typedef struct Scrollback Scrollback;

/* a container for base window and scrollback */
typedef struct ScrollbackWindow ScrollbackWindow;

ScrollbackWindow * create_scrollback_window(int rows, int cols, int startY, int startX, int multiplier);
void delete_scrollback_window(ScrollbackWindow *scrollbackWindow);

Scrollback * create_scrollback(int viewportHeight, int multiplier);
void delete_scrollback(Scrollback *scrollback);

bool is_scrollback_empty(Scrollback *scrollback);
bool is_scrollback_full(Scrollback *scrollback);

/* adds one line of text to the scrollback buffer */
void add_to_scrollback(Scrollback *scrollback, cchar_t *string);

/* moves the visible scrollback position up */
void move_sb_up(Scrollback *scrollback, int count);

/* moves the visible scrollback position down */
void move_sb_down(Scrollback *scrollback, int count);

/* calculates the remaining lines at the top end of the buffer */
int count_remaining_top_lines(Scrollback *scrollback);

/* calculates the remaining lines at the bottom end of the buffer */
int count_remaining_bottom_lines(Scrollback *scrollback);

int get_sb_head(Scrollback *scrollback);

int get_sb_topline(Scrollback *scrollback);
void set_sb_topline(Scrollback *scrollback, int topLine);

int get_sb_bottomline(Scrollback *scrollback);
void set_sb_bottomline(Scrollback *scrollback, int bottomLine);

int get_sb_capacity(Scrollback *scrollback);

bool is_sb_scrolled_up(Scrollback *scrollback);
void set_sb_is_scrolled_up(Scrollback *scrollback, bool isScrolledUp);

ScrollSubject * get_scroll_subject(Scrollback *scrollback);

/* gets a line from the scrollback buffer */
cchar_t * get_sb_buffer_line(Scrollback *scrollback, int index);

Scrollback * get_scrollback(ScrollbackWindow *scrollbackWindow);
BaseWindow * get_sb_base_window(ScrollbackWindow *scrollbackWindow);

#endif