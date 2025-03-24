#ifndef BASE_WINDOW_H
#define BASE_WINDOW_H

/* activate ncurses wide char support */
#define NCURSES_WIDECHAR 1

#include <ncursesw/curses.h>

/* enum representing different types of windows */
typedef enum {
    BASE_WINDOW,
    SCROLLBACK_WINDOW,
    INPUT_WINDOW,
    UNKNOWN_WINDOW_TYPE,
    WINDOW_TYPE_COUNT
} WindowType;

/* a base window type */
typedef struct BaseWindow BaseWindow;

/* a group consists of at least one window and possibly more. 
 * Only one window in the group is visible at any one time. 
 * Used for creating additional windows when user joins a channel. 
 * In that case, a channel chat window becomes visible instead of
 * the main window.
 */
typedef struct WindowGroup WindowGroup;

BaseWindow * create_base_window(int rows, int cols, int startY, int startX, WindowType windowType);
void delete_base_window(BaseWindow *baseWindow);

WindowGroup * create_window_group(int capacity);
void delete_window_group(WindowGroup *windowGroup);

/* add/ remove window from window group */
void add_window(WindowGroup *windowGroup, BaseWindow *baseWindow);
void remove_window(WindowGroup *windowGroup);

/* gets/ sets active window in window group */
BaseWindow * get_active_window(WindowGroup *windowGroup);
void set_active_window(WindowGroup *windowGroup, int activeWindowIndex);

WINDOW * get_window(BaseWindow *baseWindow);

WindowType get_window_type(BaseWindow *baseWindow);

int get_window_rows(BaseWindow *baseWindow);
int get_window_cols(BaseWindow *baseWindow);

#endif