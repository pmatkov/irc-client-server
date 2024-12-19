/* --INTERNAL HEADER--
    used for testing */
#ifndef SCROLLBACK_WINDOW_H
#define SCROLLBACK_WINDOW_H

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include "priv_scroll_subject.h"
#include "priv_base_window.h"

#include <ncursesw/curses.h>

typedef struct {
    cchar_t **buffer;
    int tail;
    int head;
    int topLine;
    int bottomLine;
    int viewportHeight;
    bool isScrolledUp;
    bool isMessagePending;
    ScrollSubject *subject;
    int count;
    int capacity;
} Scrollback;

typedef struct {
    BaseWindow baseWindow;
    Scrollback *scrollback;
} ScrollbackWindow;

ScrollbackWindow * create_scrollback_window(int rows, int cols, int startY, int startX, int multiplier);
void delete_scrollback_window(ScrollbackWindow *scrollbackWindow);

Scrollback * create_scrollback(int viewportHeight, int multiplier);
void delete_scrollback(Scrollback *scrollback);

bool is_scrollback_empty(Scrollback *scrollback);
bool is_scrollback_full(Scrollback *scrollback);

void add_to_scrollback(Scrollback *scrollback, cchar_t *string);

void move_sb_up(Scrollback *scrollback, int count);
void move_sb_down(Scrollback *scrollback, int count);

int count_remaining_top_lines(Scrollback *scrollback);
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
cchar_t * get_sb_buffer_line(Scrollback *scrollback, int index);

Scrollback * get_scrollback(ScrollbackWindow *scrollbackWindow);
BaseWindow * get_sb_base_window(ScrollbackWindow *scrollbackWindow);

#endif